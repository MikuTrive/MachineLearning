#include "main_window.h"

#include "app_meta.h"
#include "i18n.h"
#include "lesson_data.h"
#include "quiz_data.h"
#include "paths.h"

#include <glib/gstdio.h>
#include <gtksourceview/gtksource.h>

struct MainWindow;

typedef struct FoldCard {
  struct MainWindow *owner;
  GtkWidget *container;
  GtkWidget *title_label;
  GtkWidget *toggle_button;
  GtkWidget *toggle_image;
  GtkWidget *revealer;
  GtkWidget *content_box;
  gboolean expanded;
} FoldCard;

typedef enum {
  LESSON_RESULT_PASS,
  LESSON_RESULT_COMPILE_ERROR,
  LESSON_RESULT_RUNTIME_ERROR,
  LESSON_RESULT_OUTPUT_MISMATCH
} LessonRunResult;

struct MainWindow {
  GtkApplication *app;
  GtkWidget *window;
  GtkWidget *root;
  GtkWidget *header_title_label;
  GtkWidget *sidebar_title_label;
  GtkWidget *list_box;
  GtkWidget *banner_revealer;
  GtkWidget *banner_box;
  GtkWidget *banner_label;
  GtkWidget *page_title_label;
  GtkWidget *page_subtitle_label;
  GtkWidget *current_node_value;
  GtkWidget *database_value;
  GtkWidget *language_value;
  GtkWidget *compiler_root_value;
  GtkWidget *headers_body_label;
  GtkWidget *goal_body_label;
  GtkWidget *editor_panel;
  GtkWidget *source_view;
  GtkWidget *example_view;
  GtkWidget *editor_title_label;
  GtkWidget *editor_subtitle_label;
  GtkWidget *run_button;
  GtkWidget *run_button_label;
  GtkWidget *status_label;
  GtkWidget *terminal_card;
  GtkWidget *terminal_title_label;
  GtkWidget *terminal_empty_label;
  GtkWidget *titlebar_settings_button;
  GtkWidget *titlebar_min_button;
  GtkWidget *titlebar_max_button;
  GtkWidget *titlebar_close_button;
  GtkWidget *settings_close_button;
  GtkWidget *quiz_card;
  GtkWidget *quiz_title_label;
  GtkWidget *quiz_intro_label;
  GtkWidget *quiz_questions_box;
  GtkWidget *quiz_submit_button;
  GtkWidget *quiz_result_label;
  GtkTextBuffer *terminal_buffer;
  GtkSourceBuffer *source_buffer;
  GtkSourceBuffer *example_buffer;
  FoldCard notes_card;
  FoldCard meta_card;
  FoldCard headers_card;
  FoldCard example_card;
  FoldCard goal_card;
  const LessonSpec *current_lesson;
  gchar *current_node_id;
  gchar *chevron_up_path;
  gchar *chevron_down_path;
  gchar *gear_path;
  gchar *pending_language;
  GtkWidget *settings_window;
  GtkWidget *settings_title_label;
  GtkWidget *settings_language_label;
  GtkWidget *settings_select_button;
  GtkWidget *settings_select_label;
  GtkWidget *settings_choice_label;
  GtkWidget *settings_revealer;
  GtkWidget *settings_lang_zh_button;
  GtkWidget *settings_lang_en_button;
  GtkWidget *settings_save_button;
  guint banner_timeout_id;
  guint editor_save_timeout_id;
  gboolean ignore_buffer_change;
  gint quiz_answers[64];
  gsize quiz_question_count;
  AppConfig *config;
  AppDatabase *db;
};

static void update_for_lesson(MainWindow *self, const LessonSpec *lesson);
static void on_run_clicked(GtkButton *button, gpointer user_data);

typedef struct {
  MainWindow *self;
  guint question_index;
  guint choice_index;
} QuizChoiceData;

static gboolean lesson_is_quiz(const LessonSpec *lesson) {
  return lesson && lesson->mode == LESSON_MODE_QUIZ;
}

static const gchar *quiz_pick_text(const gchar *language, const gchar *en, const gchar *zh) {
  return g_strcmp0(app_lang_normalize(language), "zh-CN") == 0 ? zh : en;
}

static gchar quiz_choice_letter(gint index) {
  switch (index) {
    case 0: return 'A';
    case 1: return 'B';
    default: return 'C';
  }
}

static void quiz_choice_data_free(gpointer data, GClosure *closure) {
  (void)closure;
  g_free(data);
}

static void clear_box_children(GtkWidget *box) {
  GtkWidget *child = gtk_widget_get_first_child(box);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_box_remove(GTK_BOX(box), child);
    child = next;
  }
}

static void update_titlebar_tooltips(MainWindow *self) {
  if (self->titlebar_settings_button) {
    gtk_widget_set_tooltip_text(self->titlebar_settings_button, app_i18n_get(self->config->language, "titlebar.settings"));
  }
  if (self->titlebar_min_button) {
    gtk_widget_set_tooltip_text(self->titlebar_min_button, app_i18n_get(self->config->language, "titlebar.minimize"));
  }
  if (self->titlebar_max_button) {
    gtk_widget_set_tooltip_text(self->titlebar_max_button, app_i18n_get(self->config->language, "titlebar.maximize"));
  }
  if (self->titlebar_close_button) {
    gtk_widget_set_tooltip_text(self->titlebar_close_button, app_i18n_get(self->config->language, "titlebar.close"));
  }
  if (self->settings_close_button) {
    gtk_widget_set_tooltip_text(self->settings_close_button, app_i18n_get(self->config->language, "settings.close_panel"));
  }
}

static gboolean quiz_all_answered(MainWindow *self) {
  for (gsize i = 0; i < self->quiz_question_count; ++i) {
    if (self->quiz_answers[i] < 0) {
      return FALSE;
    }
  }
  return self->quiz_question_count > 0;
}

static void update_quiz_submit_state(MainWindow *self) {
  if (self->quiz_submit_button) {
    gtk_widget_set_sensitive(self->quiz_submit_button, quiz_all_answered(self));
  }
}

static void on_quiz_choice_toggled(GtkCheckButton *button, gpointer user_data) {
  QuizChoiceData *choice = user_data;
  if (!gtk_check_button_get_active(button)) {
    return;
  }
  choice->self->quiz_answers[choice->question_index] = (gint)choice->choice_index;
  update_quiz_submit_state(choice->self);
}

static void rebuild_quiz_questions(MainWindow *self) {
  const QuizQuestion *questions = NULL;
  gsize count = 0;
  clear_box_children(self->quiz_questions_box);
  questions = app_quiz_questions(&count);
  self->quiz_question_count = count;
  for (gsize i = 0; i < count; ++i) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *prompt = gtk_label_new(quiz_pick_text(self->config->language, questions[i].prompt_en, questions[i].prompt_zh));
    GtkCheckButton *group = NULL;
    gtk_widget_add_css_class(card, "panel-card");
    gtk_widget_add_css_class(prompt, "content-body");
    gtk_label_set_xalign(GTK_LABEL(prompt), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(prompt), TRUE);
    gtk_box_append(GTK_BOX(card), prompt);
    for (gsize c = 0; c < 3; ++c) {
      GtkWidget *button = gtk_check_button_new_with_label(quiz_pick_text(self->config->language, questions[i].choices_en[c], questions[i].choices_zh[c]));
      QuizChoiceData *payload = g_new0(QuizChoiceData, 1);
      payload->self = self;
      payload->question_index = (guint)i;
      payload->choice_index = (guint)c;
      if (group) {
        gtk_check_button_set_group(GTK_CHECK_BUTTON(button), group);
      } else {
        group = GTK_CHECK_BUTTON(button);
      }
      gtk_widget_add_css_class(button, "quiz-choice-button");
      g_signal_connect_data(button, "toggled", G_CALLBACK(on_quiz_choice_toggled), payload, quiz_choice_data_free, 0);
      if (self->quiz_answers[i] == (gint)c) {
        gtk_check_button_set_active(GTK_CHECK_BUTTON(button), TRUE);
      }
      gtk_box_append(GTK_BOX(card), button);
    }
    gtk_box_append(GTK_BOX(self->quiz_questions_box), card);
  }
  update_quiz_submit_state(self);
}

static void on_quiz_submit_clicked(GtkButton *button, gpointer user_data) {
  MainWindow *self = user_data;
  const QuizQuestion *questions = NULL;
  gsize count = 0;
  gint score = 0;
  GString *report = g_string_new(NULL);
  gboolean has_wrong = FALSE;
  (void)button;
  if (!quiz_all_answered(self)) {
    gtk_label_set_text(GTK_LABEL(self->quiz_result_label), app_i18n_get(self->config->language, "quiz.answer_all"));
    return;
  }
  questions = app_quiz_questions(&count);
  for (gsize i = 0; i < count; ++i) {
    if (self->quiz_answers[i] == questions[i].correct_index) {
      score += 2;
    } else {
      has_wrong = TRUE;
    }
  }
  g_string_append_printf(report, app_i18n_get(self->config->language, "quiz.result_format"), score);
  if (!has_wrong) {
    g_string_append_printf(report, "\n\n%s", app_i18n_get(self->config->language, "quiz.all_correct"));
  } else {
    g_string_append_printf(report, "\n\n%s\n", app_i18n_get(self->config->language, "quiz.feedback_header"));
    for (gsize i = 0; i < count; ++i) {
      if (self->quiz_answers[i] != questions[i].correct_index) {
        g_autofree gchar *question_label = g_strdup_printf(app_i18n_get(self->config->language, "quiz.q_format"), (gint)(i + 1));
        g_string_append_printf(report,
                               "\n%s\n%s: %c\n%s: %c\n%s\n",
                               question_label,
                               app_i18n_get(self->config->language, "quiz.your_answer"),
                               quiz_choice_letter(self->quiz_answers[i]),
                               app_i18n_get(self->config->language, "quiz.correct_answer"),
                               quiz_choice_letter(questions[i].correct_index),
                               quiz_pick_text(self->config->language, questions[i].explain_en, questions[i].explain_zh));
      }
    }
  }
  gtk_label_set_text(GTK_LABEL(self->quiz_result_label), report->str);
  g_string_free(report, TRUE);
}

static GtkWidget *create_quiz_card(MainWindow *self) {
  GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  GtkWidget *submit_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  self->quiz_card = card;
  self->quiz_title_label = gtk_label_new(app_i18n_get(self->config->language, "quiz.title"));
  self->quiz_intro_label = gtk_label_new(app_i18n_get(self->config->language, "quiz.subtitle"));
  self->quiz_questions_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  self->quiz_submit_button = gtk_button_new_with_label(app_i18n_get(self->config->language, "quiz.submit"));
  self->quiz_result_label = gtk_label_new(app_i18n_get(self->config->language, "quiz.ready"));
  gtk_widget_add_css_class(card, "panel-card");
  gtk_widget_add_css_class(self->quiz_title_label, "section-title");
  gtk_widget_add_css_class(self->quiz_intro_label, "muted-label");
  gtk_widget_add_css_class(self->quiz_result_label, "content-body");
  gtk_widget_add_css_class(self->quiz_submit_button, "accent-button");
  gtk_label_set_xalign(GTK_LABEL(self->quiz_title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(self->quiz_intro_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(self->quiz_result_label), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(self->quiz_intro_label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(self->quiz_result_label), TRUE);
  gtk_box_append(GTK_BOX(card), self->quiz_title_label);
  gtk_box_append(GTK_BOX(card), self->quiz_intro_label);
  gtk_box_append(GTK_BOX(card), self->quiz_questions_box);
  gtk_box_append(GTK_BOX(submit_row), self->quiz_submit_button);
  gtk_box_append(GTK_BOX(card), submit_row);
  gtk_box_append(GTK_BOX(card), self->quiz_result_label);
  g_signal_connect(self->quiz_submit_button, "clicked", G_CALLBACK(on_quiz_submit_clicked), self);
  rebuild_quiz_questions(self);
  gtk_widget_set_visible(card, FALSE);
  return card;
}

static const gchar *machine_demo_code(void) {
  return "bin.runtime\n"
         "struct LessonCard:\n"
         "  title: str\n"
         "  done: bool\n"
         "\n"
         "main:\n"
         "  var card = LessonCard()\n"
         "  card.title = \"Machine Learning\"\n"
         "  card.done = true\n"
         "  print card.title\n"
         "  print card.done\n"
         "  ret 0\n";
}

static gchar *buffer_get_all_text(GtkTextBuffer *buffer) {
  GtkTextIter start;
  GtkTextIter end;
  gtk_text_buffer_get_bounds(buffer, &start, &end);
  return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static gchar *build_editor_key(const gchar *node_id) {
  return g_strdup_printf("editor.%s", node_id ? node_id : "overview.all");
}

static gchar *build_workspace_dir(const AppConfig *config) {
  if (!config || !config->db_path || *config->db_path == '\0') {
    return NULL;
  }
  return g_path_get_dirname(config->db_path);
}

static gchar *build_workspace_basename(const gchar *node_id, const gchar *suffix) {
  g_autofree gchar *safe = g_strdup(node_id ? node_id : "lesson");
  for (gchar *cursor = safe; cursor && *cursor != '\0'; ++cursor) {
    if (!g_ascii_isalnum(*cursor)) {
      *cursor = '_';
    }
  }
  return g_strdup_printf("%s%s", safe, suffix ? suffix : "");
}

static gchar *build_workspace_source_path(const AppConfig *config, const gchar *node_id) {
  g_autofree gchar *dir = build_workspace_dir(config);
  g_autofree gchar *name = build_workspace_basename(node_id, ".mne");
  return (dir && name) ? g_build_filename(dir, name, NULL) : NULL;
}

static gchar *build_workspace_binary_path(const AppConfig *config, const gchar *node_id) {
  g_autofree gchar *dir = build_workspace_dir(config);
  g_autofree gchar *name = build_workspace_basename(node_id, "_app");
  return (dir && name) ? g_build_filename(dir, name, NULL) : NULL;
}

static void terminal_set_text(MainWindow *self, const gchar *text) {
  gtk_text_buffer_set_text(self->terminal_buffer, text ? text : "", -1);
  gtk_widget_set_visible(self->terminal_empty_label, text == NULL || *text == '\0');
}

static void remove_banner_classes(MainWindow *self) {
  gtk_widget_remove_css_class(self->banner_box, "banner-success");
  gtk_widget_remove_css_class(self->banner_box, "banner-warning");
  gtk_widget_remove_css_class(self->banner_box, "banner-error");
}

static gboolean on_banner_hide_timeout(gpointer user_data) {
  MainWindow *self = user_data;
  self->banner_timeout_id = 0;
  gtk_revealer_set_reveal_child(GTK_REVEALER(self->banner_revealer), FALSE);
  return G_SOURCE_REMOVE;
}

static void show_banner(MainWindow *self, const gchar *css_class, const gchar *message) {
  if (self->banner_timeout_id != 0) {
    g_source_remove(self->banner_timeout_id);
    self->banner_timeout_id = 0;
  }
  remove_banner_classes(self);
  gtk_widget_add_css_class(self->banner_box, css_class);
  gtk_label_set_text(GTK_LABEL(self->banner_label), message);
  gtk_revealer_set_reveal_child(GTK_REVEALER(self->banner_revealer), TRUE);
  self->banner_timeout_id = g_timeout_add(1900, on_banner_hide_timeout, self);
}

static void persist_current_node(MainWindow *self, const gchar *node_id) {
  g_autoptr(GError) error = NULL;
  app_db_set_string(self->db, "current_node", node_id, &error);
  app_db_set_string(self->db, "language", app_lang_normalize(self->config->language), &error);
}

static const gchar *default_code_for_lesson(const LessonSpec *lesson) {
  if (!lesson) {
    return machine_demo_code();
  }
  if (lesson->editor_default && *lesson->editor_default) {
    return lesson->editor_default;
  }
  return lesson->challenge ? "" : machine_demo_code();
}

static gboolean save_editor_code_now(MainWindow *self) {
  g_autofree gchar *text = NULL;
  g_autofree gchar *key = NULL;
  g_autofree gchar *workspace_source_path = NULL;
  g_autofree gchar *workspace_dir = NULL;
  g_autoptr(GError) error = NULL;
  gboolean db_ok = TRUE;
  if (!self->current_node_id) {
    return TRUE;
  }
  text = buffer_get_all_text(GTK_TEXT_BUFFER(self->source_buffer));
  key = build_editor_key(self->current_node_id);
  db_ok = app_db_set_string(self->db, key, text ? text : "", &error);
  workspace_source_path = build_workspace_source_path(self->config, self->current_node_id);
  if (workspace_source_path) {
    workspace_dir = g_path_get_dirname(workspace_source_path);
    if (workspace_dir) {
      g_mkdir_with_parents(workspace_dir, 0755);
    }
    g_clear_error(&error);
    g_file_set_contents(workspace_source_path, text ? text : "", -1, &error);
  }
  return db_ok;
}

static gboolean on_editor_save_timeout(gpointer user_data) {
  MainWindow *self = user_data;
  self->editor_save_timeout_id = 0;
  save_editor_code_now(self);
  return G_SOURCE_REMOVE;
}

static void queue_editor_save(MainWindow *self) {
  if (self->ignore_buffer_change) {
    return;
  }
  if (self->editor_save_timeout_id != 0) {
    g_source_remove(self->editor_save_timeout_id);
  }
  self->editor_save_timeout_id = g_timeout_add(350, on_editor_save_timeout, self);
}

static void flush_editor_save(MainWindow *self) {
  if (self->editor_save_timeout_id != 0) {
    g_source_remove(self->editor_save_timeout_id);
    self->editor_save_timeout_id = 0;
    save_editor_code_now(self);
  }
}

static void load_editor_for_lesson(MainWindow *self, const LessonSpec *lesson) {
  g_autofree gchar *key = NULL;
  g_autofree gchar *saved = NULL;
  GtkTextIter start;
  key = build_editor_key(lesson->id);
  saved = app_db_get_string(self->db, key);
  self->ignore_buffer_change = TRUE;
  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->source_buffer), saved ? saved : default_code_for_lesson(lesson), -1);
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(self->source_buffer), &start);
  gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(self->source_buffer), &start);
  self->ignore_buffer_change = FALSE;
}

static gchar *combine_output(const gchar *stdout_text, const gchar *stderr_text) {
  const gboolean has_stdout = stdout_text && *stdout_text;
  const gboolean has_stderr = stderr_text && *stderr_text;
  if (has_stdout && has_stderr) {
    return g_str_has_suffix(stdout_text, "\n")
             ? g_strdup_printf("%s%s", stdout_text, stderr_text)
             : g_strdup_printf("%s\n%s", stdout_text, stderr_text);
  }
  if (has_stdout) {
    return g_strdup(stdout_text);
  }
  if (has_stderr) {
    return g_strdup(stderr_text);
  }
  return g_strdup("");
}

static gboolean spawn_and_capture(const gchar *cwd,
                                  const gchar *const *argv,
                                  gboolean *successful_out,
                                  gint *exit_status_out,
                                  gchar **stdout_out,
                                  gchar **stderr_out,
                                  GError **error) {
  g_autoptr(GSubprocessLauncher) launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE);
  g_autoptr(GSubprocess) process = NULL;
  if (cwd) {
    g_subprocess_launcher_set_cwd(launcher, cwd);
  }
  process = g_subprocess_launcher_spawnv(launcher, argv, error);
  if (!process) {
    return FALSE;
  }
  if (!g_subprocess_communicate_utf8(process, NULL, NULL, stdout_out, stderr_out, error)) {
    return FALSE;
  }
  if (successful_out) {
    *successful_out = g_subprocess_get_successful(process);
  }
  if (exit_status_out) {
    *exit_status_out = g_subprocess_get_exit_status(process);
  }
  return TRUE;
}

static gchar *normalize_for_compare(const gchar *text) {
  g_autofree gchar *copy = g_strdup(text ? text : "");
  g_strdelimit(copy, "\r", '\n');
  g_strchomp(copy);
  return g_strdup(g_strstrip(copy));
}

static gchar *strip_ansi_sequences(const gchar *text) {
  GString *clean = g_string_new(NULL);
  const gchar *cursor = text ? text : "";
  while (*cursor != '\0') {
    if (*cursor == '\033' && *(cursor + 1) == '[') {
      cursor += 2;
      while (*cursor != '\0' && !g_ascii_isalpha(*cursor)) {
        ++cursor;
      }
      if (*cursor != '\0') {
        ++cursor;
      }
      continue;
    }
    g_string_append_c(clean, *cursor);
    ++cursor;
  }
  return g_string_free(clean, FALSE);
}

static gchar *prepare_source_for_compile(const gchar *text) {
  GString *normalized = g_string_new(NULL);
  const gchar *cursor = text ? text : "";
  while (*cursor != '\0') {
    if (*cursor == '\r') {
      ++cursor;
      continue;
    }
    if (*cursor == '\t') {
      g_string_append(normalized, "  ");
      ++cursor;
      continue;
    }
    g_string_append_c(normalized, *cursor);
    ++cursor;
  }
  {
    gchar **lines = g_strsplit(normalized->str, "\n", -1);
    GString *rebuilt = g_string_new(NULL);
    for (gint index = 0; lines[index] != NULL; ++index) {
      g_autofree gchar *line = g_strdup(lines[index]);
      gsize len = strlen(line);
      while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[--len] = '\0';
      }
      if (len > 0 && line[len - 1] == ';') {
        line[--len] = '\0';
      }
      g_string_append(rebuilt, line);
      if (lines[index + 1] != NULL) {
        g_string_append_c(rebuilt, '\n');
      }
    }
    g_strfreev(lines);
    g_string_free(normalized, TRUE);
    normalized = rebuilt;
  }
  if (normalized->len == 0 || normalized->str[normalized->len - 1] != '\n') {
    g_string_append_c(normalized, '\n');
  }
  return g_string_free(normalized, FALSE);
}

static gchar *build_missing_items_text(MainWindow *self, const LessonSpec *lesson, const gchar *source) {
  GString *text = NULL;
  if (!lesson) {
    return NULL;
  }
  for (gint i = 0; i < 16 && lesson->required_tokens[i] != NULL; ++i) {
    const gchar *token = lesson->required_tokens[i];
    if (!g_strstr_len(source ? source : "", -1, token)) {
      if (!text) {
        text = g_string_new(app_i18n_get(self->config->language, "run.missing_items"));
      }
      g_string_append_printf(text, "\n- %s", token);
    }
  }
  return text ? g_string_free(text, FALSE) : NULL;
}

static LessonRunResult run_current_lesson(MainWindow *self,
                                          gchar **terminal_output_out,
                                          gchar **banner_message_out) {
  g_autofree gchar *machine_bin = NULL;
  g_autofree gchar *workspace_dir = NULL;
  g_autofree gchar *source_path = NULL;
  g_autofree gchar *binary_path = NULL;
  g_autofree gchar *source_text = NULL;
  g_autofree gchar *compile_text = NULL;
  g_autofree gchar *machine_dir = NULL;
  g_autofree gchar *machine_build_runtime = NULL;
  g_autofree gchar *compile_stdout = NULL;
  g_autofree gchar *compile_stderr = NULL;
  g_autofree gchar *run_stdout = NULL;
  g_autofree gchar *run_stderr = NULL;
  g_autofree gchar *combined_compile_output = NULL;
  g_autofree gchar *combined_run_output = NULL;
  g_autofree gchar *actual_output = NULL;
  g_autofree gchar *expected_output = NULL;
  g_autofree gchar *missing_items = NULL;
  g_autofree gchar *saved_files_text = NULL;
  g_autoptr(GError) error = NULL;
  gboolean compile_ok = FALSE;
  gboolean run_ok = FALSE;
  gint exit_status = 0;
  gint compile_ms = 0;
  gint run_ms = 0;
  gint64 started_us = 0;
  const gchar *compile_argv[] = {NULL, NULL, "-o", NULL, NULL};
  const gchar *run_argv[] = {NULL, NULL};
  const gchar *compile_cwd = NULL;

  if (terminal_output_out) {
    *terminal_output_out = NULL;
  }
  if (banner_message_out) {
    *banner_message_out = NULL;
  }
  if (!self->current_lesson || !self->current_lesson->challenge) {
    if (terminal_output_out) {
      *terminal_output_out = g_strdup("");
    }
    return LESSON_RESULT_OUTPUT_MISMATCH;
  }

  source_text = buffer_get_all_text(GTK_TEXT_BUFFER(self->source_buffer));
  compile_text = prepare_source_for_compile(source_text);
  machine_bin = g_find_program_in_path("machine");
  if (!machine_bin && app_paths_file_exists("/usr/local/bin/machine")) {
    machine_bin = g_strdup("/usr/local/bin/machine");
  }
  if (!machine_bin) {
    *terminal_output_out = g_strdup(app_i18n_get(self->config->language, "run.machine_missing"));
    return LESSON_RESULT_COMPILE_ERROR;
  }

  workspace_dir = build_workspace_dir(self->config);
  source_path = build_workspace_source_path(self->config, self->current_lesson->id);
  binary_path = build_workspace_binary_path(self->config, self->current_lesson->id);
  if (!workspace_dir || !source_path || !binary_path) {
    *terminal_output_out = g_strdup(app_i18n_get(self->config->language, "run.tempdir_failed"));
    return LESSON_RESULT_COMPILE_ERROR;
  }
  if (g_mkdir_with_parents(workspace_dir, 0755) != 0) {
    *terminal_output_out = g_strdup(app_i18n_get(self->config->language, "run.tempdir_failed"));
    return LESSON_RESULT_COMPILE_ERROR;
  }
  g_remove(binary_path);
  if (!g_file_set_contents(source_path, compile_text ? compile_text : "", -1, &error)) {
    *terminal_output_out = g_strdup(error ? error->message : app_i18n_get(self->config->language, "run.write_failed"));
    return LESSON_RESULT_COMPILE_ERROR;
  }

  saved_files_text = g_strdup_printf("%s:\n%s\n%s\n",
                                     app_i18n_get(self->config->language, "run.saved_files"),
                                     source_path,
                                     binary_path);
  machine_dir = g_path_get_dirname(machine_bin);
  machine_build_runtime = g_build_filename(machine_dir, "build", "machine_runtime.o", NULL);
  compile_cwd = workspace_dir;
  if (app_paths_file_exists("/usr/local/lib/machine/machine_runtime.o") &&
      app_paths_file_exists("/usr/local/include/machine/machine_runtime.h")) {
    compile_cwd = "/usr/local/lib/machine";
  } else if (app_paths_file_exists(machine_build_runtime)) {
    compile_cwd = machine_dir;
  }

  compile_argv[0] = machine_bin;
  compile_argv[1] = source_path;
  compile_argv[3] = binary_path;
  started_us = g_get_monotonic_time();
  if (!spawn_and_capture(compile_cwd, compile_argv, &compile_ok, &exit_status,
                         &compile_stdout, &compile_stderr, &error)) {
    *terminal_output_out = g_strdup_printf("%s\n\n%s",
                                           error ? error->message : app_i18n_get(self->config->language, "run.compile_failed"),
                                           saved_files_text);
    return LESSON_RESULT_COMPILE_ERROR;
  }
  compile_ms = (gint)((g_get_monotonic_time() - started_us) / 1000);
  combined_compile_output = strip_ansi_sequences(combine_output(compile_stdout, compile_stderr));
  if (!compile_ok) {
    *terminal_output_out = g_strdup_printf("%s: %d ms\n\n%s\n%d\n\n%s\n\n%s",
                                           app_i18n_get(self->config->language, "run.compile_time"),
                                           compile_ms,
                                           app_i18n_get(self->config->language, "run.compile_exit"),
                                           exit_status,
                                           combined_compile_output ? combined_compile_output : "",
                                           saved_files_text);
    return LESSON_RESULT_COMPILE_ERROR;
  }

  run_argv[0] = binary_path;
  started_us = g_get_monotonic_time();
  if (!spawn_and_capture(workspace_dir, run_argv, &run_ok, &exit_status,
                         &run_stdout, &run_stderr, &error)) {
    *terminal_output_out = g_strdup_printf("%s\n\n%s",
                                           error ? error->message : app_i18n_get(self->config->language, "run.execute_failed"),
                                           saved_files_text);
    return LESSON_RESULT_RUNTIME_ERROR;
  }
  run_ms = (gint)((g_get_monotonic_time() - started_us) / 1000);
  combined_run_output = strip_ansi_sequences(combine_output(run_stdout, run_stderr));
  actual_output = normalize_for_compare(run_stdout);
  expected_output = normalize_for_compare(self->current_lesson->expected_output);
  missing_items = build_missing_items_text(self, self->current_lesson, compile_text);
  if (!run_ok) {
    *terminal_output_out = g_strdup_printf("%s\n\n%s: %d ms\n%s: %d ms\n\n%s",
                                           combined_run_output ? combined_run_output : "",
                                           app_i18n_get(self->config->language, "run.compile_time"),
                                           compile_ms,
                                           app_i18n_get(self->config->language, "run.run_time"),
                                           run_ms,
                                           saved_files_text);
    return LESSON_RESULT_RUNTIME_ERROR;
  }
  if (missing_items) {
    *terminal_output_out = g_strdup_printf("%s\n\n%s\n\n%s: %d ms\n%s: %d ms\n\n%s",
                                           combined_run_output ? combined_run_output : "",
                                           missing_items,
                                           app_i18n_get(self->config->language, "run.compile_time"),
                                           compile_ms,
                                           app_i18n_get(self->config->language, "run.run_time"),
                                           run_ms,
                                           saved_files_text);
    return LESSON_RESULT_OUTPUT_MISMATCH;
  }

  {
    gboolean output_ok = FALSE;
    if (!self->current_lesson->expected_output || *(self->current_lesson->expected_output) == '\0') {
      output_ok = TRUE;
    } else if (g_strcmp0(actual_output, expected_output) == 0 ||
               (actual_output && expected_output && g_strstr_len(actual_output, -1, expected_output) != NULL)) {
      output_ok = TRUE;
    }
    *terminal_output_out = g_strdup_printf("%s\n\n%s: %d ms\n%s: %d ms\n\n%s",
                                           combined_run_output ? combined_run_output : "",
                                           app_i18n_get(self->config->language, "run.compile_time"),
                                           compile_ms,
                                           app_i18n_get(self->config->language, "run.run_time"),
                                           run_ms,
                                           saved_files_text);
    if (!output_ok) {
      return LESSON_RESULT_OUTPUT_MISMATCH;
    }
  }

  switch (self->current_lesson->mode) {
    case LESSON_MODE_BENCH_COMPILE:
      if (compile_ms <= self->current_lesson->compile_target_ms) {
        if (banner_message_out) {
          *banner_message_out = g_strdup_printf(app_i18n_get(self->config->language, "banner.compile_speed_pass"), compile_ms);
        }
      } else {
        if (banner_message_out) {
          *banner_message_out = g_strdup_printf(app_i18n_get(self->config->language, "banner.compile_speed_fail"), compile_ms);
        }
        return LESSON_RESULT_OUTPUT_MISMATCH;
      }
      break;
    case LESSON_MODE_BENCH_RUN:
      if (run_ms <= self->current_lesson->run_target_ms) {
        if (banner_message_out) {
          *banner_message_out = g_strdup_printf(app_i18n_get(self->config->language, "banner.run_speed_pass"), run_ms);
        }
      } else {
        if (banner_message_out) {
          *banner_message_out = g_strdup_printf(app_i18n_get(self->config->language, "banner.run_speed_fail"), run_ms);
        }
        return LESSON_RESULT_OUTPUT_MISMATCH;
      }
      break;
    case LESSON_MODE_BENCH_PROGRAM:
      if (compile_ms <= self->current_lesson->compile_target_ms && run_ms <= self->current_lesson->run_target_ms) {
        if (banner_message_out) {
          *banner_message_out = g_strdup_printf(app_i18n_get(self->config->language, "banner.program_speed_pass"), compile_ms, run_ms);
        }
      } else {
        if (banner_message_out) {
          *banner_message_out = g_strdup_printf(app_i18n_get(self->config->language, "banner.program_speed_fail"), compile_ms, run_ms);
        }
        return LESSON_RESULT_OUTPUT_MISMATCH;
      }
      break;
    case LESSON_MODE_INFO:
    case LESSON_MODE_OUTPUT:
    default:
      break;
  }

  {
    g_autoptr(GError) db_error = NULL;
    g_autofree gchar *pass_key = g_strdup_printf("pass.%s", self->current_lesson->id);
    app_db_set_int(self->db, pass_key, 1, &db_error);
  }
  return LESSON_RESULT_PASS;
}

static void on_run_clicked(GtkButton *button, gpointer user_data) {
  MainWindow *self = user_data;
  g_autofree gchar *terminal_output = NULL;
  g_autofree gchar *banner_message = NULL;
  LessonRunResult result;
  (void)button;

  flush_editor_save(self);
  result = run_current_lesson(self, &terminal_output, &banner_message);
  terminal_set_text(self, terminal_output);

  switch (result) {
    case LESSON_RESULT_PASS:
      show_banner(self, "banner-success",
                  banner_message ? banner_message : app_i18n_get(self->config->language, "banner.passed"));
      break;
    case LESSON_RESULT_COMPILE_ERROR:
      show_banner(self, "banner-error",
                  banner_message ? banner_message : app_i18n_get(self->config->language, "banner.compile_error"));
      break;
    case LESSON_RESULT_RUNTIME_ERROR:
      show_banner(self, "banner-error",
                  banner_message ? banner_message : app_i18n_get(self->config->language, "banner.runtime_error"));
      break;
    case LESSON_RESULT_OUTPUT_MISMATCH:
    default:
      show_banner(self, "banner-warning",
                  banner_message ? banner_message : app_i18n_get(self->config->language, "banner.output_mismatch"));
      break;
  }
}

static GtkWidget *create_section_label(const gchar *text) {
  GtkWidget *label = gtk_label_new(text);
  gtk_widget_add_css_class(label, "section-label");
  gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
  return label;
}

static GtkWidget *create_value_row(const gchar *title, GtkWidget **value_label_out) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  GtkWidget *title_label = gtk_label_new(title);
  GtkWidget *value_label = gtk_label_new("");
  gtk_widget_add_css_class(box, "info-row");
  gtk_widget_add_css_class(title_label, "muted-label");
  gtk_widget_add_css_class(value_label, "value-label");
  gtk_label_set_xalign(GTK_LABEL(title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(value_label), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(value_label), TRUE);
  gtk_box_append(GTK_BOX(box), title_label);
  gtk_box_append(GTK_BOX(box), value_label);
  if (value_label_out) {
    *value_label_out = value_label;
  }
  return box;
}

static void fold_card_apply_state(FoldCard *card, gboolean expanded) {
  MainWindow *self = card->owner;
  GtkWidget *image = gtk_image_new_from_file(expanded ? self->chevron_up_path : self->chevron_down_path);
  card->expanded = expanded;
  gtk_revealer_set_reveal_child(GTK_REVEALER(card->revealer), expanded);
  gtk_widget_set_tooltip_text(card->toggle_button,
                              app_i18n_get(self->config->language, expanded ? "module.toggle_collapse" : "module.toggle_expand"));
  card->toggle_image = image;
  gtk_button_set_child(GTK_BUTTON(card->toggle_button), image);
}

static void on_fold_toggle_clicked(GtkButton *button, gpointer user_data) {
  FoldCard *card = user_data;
  (void)button;
  fold_card_apply_state(card, !card->expanded);
}

static GtkWidget *create_fold_card(MainWindow *self, FoldCard *card, const gchar *title) {
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  card->owner = self;
  card->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  card->title_label = gtk_label_new(title);
  card->toggle_button = gtk_button_new();
  card->toggle_image = gtk_image_new_from_file(self->chevron_down_path);
  card->revealer = gtk_revealer_new();
  card->content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_add_css_class(card->container, "panel-card");
  gtk_widget_add_css_class(card->title_label, "section-title");
  gtk_widget_add_css_class(card->toggle_button, "fold-toggle-button");
  gtk_widget_add_css_class(card->content_box, "fold-content");
  gtk_label_set_xalign(GTK_LABEL(card->title_label), 0.0f);
  gtk_widget_set_hexpand(card->title_label, TRUE);
  gtk_revealer_set_transition_type(GTK_REVEALER(card->revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
  gtk_revealer_set_transition_duration(GTK_REVEALER(card->revealer), 220);
  gtk_button_set_child(GTK_BUTTON(card->toggle_button), card->toggle_image);
  gtk_box_append(GTK_BOX(header), card->title_label);
  gtk_box_append(GTK_BOX(header), card->toggle_button);
  gtk_revealer_set_child(GTK_REVEALER(card->revealer), card->content_box);
  gtk_box_append(GTK_BOX(card->container), header);
  gtk_box_append(GTK_BOX(card->container), card->revealer);
  g_signal_connect(card->toggle_button, "clicked", G_CALLBACK(on_fold_toggle_clicked), card);
  fold_card_apply_state(card, FALSE);
  return card->container;
}

static void on_source_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
  (void)buffer;
  MainWindow *self = user_data;
  queue_editor_save(self);
}

static void update_card_titles(MainWindow *self) {
  gtk_label_set_text(GTK_LABEL(self->notes_card.title_label), app_i18n_get(self->config->language, "module.lesson_notes"));
  gtk_label_set_text(GTK_LABEL(self->meta_card.title_label), app_i18n_get(self->config->language, "module.current_node"));
  gtk_label_set_text(GTK_LABEL(self->headers_card.title_label), app_i18n_get(self->config->language, "module.headers"));
  gtk_label_set_text(GTK_LABEL(self->example_card.title_label), app_i18n_get(self->config->language, "module.example_code"));
  gtk_label_set_text(GTK_LABEL(self->goal_card.title_label), app_i18n_get(self->config->language, "module.goal"));
}

static void update_editor_state(MainWindow *self, const LessonSpec *lesson) {
  const gboolean challenge = lesson && lesson->challenge;
  const gboolean quiz = lesson_is_quiz(lesson);
  gtk_widget_set_visible(self->editor_panel, !quiz);
  gtk_widget_set_visible(self->run_button, challenge && !quiz);
  gtk_widget_set_visible(self->goal_card.container, challenge && !quiz);
  gtk_widget_set_visible(self->terminal_card, challenge && !quiz);
  if (self->source_view) {
    gtk_text_view_set_editable(GTK_TEXT_VIEW(self->source_view), !quiz);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(self->source_view), !quiz);
  }
  gtk_label_set_text(GTK_LABEL(self->editor_title_label), app_i18n_get(self->config->language, "list1.editor_title"));
  gtk_label_set_text(GTK_LABEL(self->editor_subtitle_label), app_i18n_get(self->config->language, "list1.editor_subtitle"));
  gtk_label_set_text(GTK_LABEL(self->run_button_label), app_i18n_get(self->config->language, "list1.run"));
  gtk_label_set_text(GTK_LABEL(self->status_label),
                     app_i18n_get(self->config->language, challenge ? "list1.status_autosave" : "list1.status_resume"));
  if (!challenge || quiz) {
    terminal_set_text(self, "");
  }
}

static void update_example_view(MainWindow *self, const LessonSpec *lesson) {
  const gchar *text = (lesson && lesson->example_code && *lesson->example_code)
                        ? lesson->example_code
                        : app_i18n_get(self->config->language, "module.example_empty");
  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->example_buffer), text, -1);
}

static void update_for_lesson(MainWindow *self, const LessonSpec *lesson) {
  const gchar *language_name = g_strcmp0(app_lang_normalize(self->config->language), "zh-CN") == 0
                                 ? app_i18n_get(self->config->language, "wizard.language.zh_cn")
                                 : app_i18n_get(self->config->language, "wizard.language.english");
  g_free(self->current_node_id);
  self->current_node_id = g_strdup(lesson->id);
  self->current_lesson = lesson;
  gtk_label_set_text(GTK_LABEL(self->page_title_label), app_lesson_title(lesson, self->config->language));
  gtk_label_set_text(GTK_LABEL(self->page_subtitle_label), app_lesson_summary(lesson, self->config->language));
  gtk_label_set_text(GTK_LABEL(self->current_node_value), lesson->slug);
  gtk_label_set_text(GTK_LABEL(self->database_value), self->config->db_path ? self->config->db_path : "");
  gtk_label_set_text(GTK_LABEL(self->language_value), language_name);
  gtk_label_set_text(GTK_LABEL(self->compiler_root_value), "/usr/local/lib/machine");
  gtk_label_set_text(GTK_LABEL(self->headers_body_label), app_lesson_headers(lesson, self->config->language));
  gtk_label_set_text(GTK_LABEL(self->goal_body_label), app_lesson_goal(lesson, self->config->language));
  update_card_titles(self);
  update_editor_state(self, lesson);
  update_example_view(self, lesson);
  gtk_widget_set_visible(self->quiz_card, lesson_is_quiz(lesson));
  gtk_widget_set_visible(self->notes_card.container, !lesson_is_quiz(lesson));
  gtk_widget_set_visible(self->meta_card.container, !lesson_is_quiz(lesson));
  gtk_widget_set_visible(self->headers_card.container, !lesson_is_quiz(lesson));
  gtk_widget_set_visible(self->example_card.container, !lesson_is_quiz(lesson));
  fold_card_apply_state(&self->notes_card, FALSE);
  fold_card_apply_state(&self->meta_card, FALSE);
  fold_card_apply_state(&self->headers_card, FALSE);
  fold_card_apply_state(&self->example_card, FALSE);
  fold_card_apply_state(&self->goal_card, FALSE);
  if (lesson_is_quiz(lesson)) {
    rebuild_quiz_questions(self);
    gtk_label_set_text(GTK_LABEL(self->quiz_result_label), app_i18n_get(self->config->language, "quiz.ready"));
  }
  {
    GtkWidget *child = gtk_widget_get_first_child(self->notes_card.content_box);
    while (child) {
      GtkWidget *next = gtk_widget_get_next_sibling(child);
      gtk_box_remove(GTK_BOX(self->notes_card.content_box), child);
      child = next;
    }
    GtkWidget *notes = gtk_label_new(app_lesson_notes(lesson, self->config->language));
    gtk_widget_add_css_class(notes, "content-body");
    gtk_label_set_xalign(GTK_LABEL(notes), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(notes), TRUE);
    gtk_box_append(GTK_BOX(self->notes_card.content_box), notes);
  }
  load_editor_for_lesson(self, lesson);
  persist_current_node(self, lesson->id);
}

static void on_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
  MainWindow *self = user_data;
  const gchar *node_id = NULL;
  (void)box;
  if (!row) {
    return;
  }
  flush_editor_save(self);
  node_id = g_object_get_data(G_OBJECT(row), "node-id");
  update_for_lesson(self, app_lesson_find(node_id));
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
  MainWindow *self = user_data;
  (void)widget;
  self->window = NULL;
}

static gboolean on_window_close(GtkWindow *window, gpointer user_data) {
  MainWindow *self = user_data;
  GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->list_box));
  const gchar *node_id = selected_row ? g_object_get_data(G_OBJECT(selected_row), "node-id") : "overview.all";
  (void)window;
  flush_editor_save(self);
  persist_current_node(self, node_id);
  return FALSE;
}

static void on_titlebar_minimize_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  gtk_window_minimize(GTK_WINDOW(user_data));
}

static void on_titlebar_maximize_clicked(GtkButton *button, gpointer user_data) {
  GtkWindow *window = GTK_WINDOW(user_data);
  (void)button;
  if (gtk_window_is_maximized(window)) {
    gtk_window_unmaximize(window);
  } else {
    gtk_window_maximize(window);
  }
}

static void on_titlebar_close_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  gtk_window_close(GTK_WINDOW(user_data));
}

static GtkWidget *create_titlebar_dot_button(const gchar *icon_name,
                                             const gchar *tooltip,
                                             GCallback callback,
                                             gpointer user_data) {
  g_autofree gchar *icon_path = g_build_filename(app_paths_get_data_dir(), "titlebar-icons", icon_name, NULL);
  GtkWidget *button = gtk_button_new();
  GtkWidget *image = gtk_image_new_from_file(icon_path);
  gtk_widget_add_css_class(button, "titlebar-dot-button");
  gtk_widget_set_size_request(button, 14, 14);
  gtk_button_set_child(GTK_BUTTON(button), image);
  gtk_widget_set_tooltip_text(button, tooltip);
  g_signal_connect(button, "clicked", callback, user_data);
  return button;
}

static gint lesson_group_rank(LessonDifficulty difficulty) {
  switch (difficulty) {
    case LESSON_DIFFICULTY_NONE:
      return 0;
    case LESSON_DIFFICULTY_NORMAL:
      return 1;
    case LESSON_DIFFICULTY_HARD:
      return 2;
    case LESSON_DIFFICULTY_EXTREME:
      return 3;
    case LESSON_DIFFICULTY_ADVANCED:
      return 4;
    default:
      return 5;
  }
}

static void add_lesson_row(MainWindow *self, const LessonSpec *lesson) {
  GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  GtkWidget *title_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *row_title = gtk_label_new(app_lesson_title(lesson, self->config->language));
  GtkWidget *row_hint = gtk_label_new(lesson->slug);
  GtkWidget *row = gtk_list_box_row_new();
  const gchar *badge_text = app_lesson_difficulty_text(lesson->difficulty, self->config->language);
  const gchar *badge_css = app_lesson_difficulty_css_class(lesson->difficulty);
  gtk_widget_add_css_class(row, "lesson-row");
  gtk_widget_add_css_class(title_row, "lesson-title-row");
  gtk_widget_add_css_class(row_title, "lesson-title");
  gtk_widget_add_css_class(row_hint, "lesson-hint");
  gtk_label_set_xalign(GTK_LABEL(row_title), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(row_hint), 0.0f);
  gtk_widget_set_hexpand(row_title, TRUE);
  gtk_box_append(GTK_BOX(title_row), row_title);
  if (badge_text && *badge_text) {
    GtkWidget *badge = gtk_label_new(badge_text);
    gtk_widget_add_css_class(badge, "lesson-badge");
    if (badge_css) {
      gtk_widget_add_css_class(badge, badge_css);
    }
    gtk_box_append(GTK_BOX(title_row), badge);
  }
  gtk_box_append(GTK_BOX(row_box), title_row);
  gtk_box_append(GTK_BOX(row_box), row_hint);
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
  g_object_set_data_full(G_OBJECT(row), "node-id", g_strdup(lesson->id), g_free);
  gtk_list_box_append(GTK_LIST_BOX(self->list_box), row);
}

static void populate_lesson_list(MainWindow *self, const gchar *selected_node_id) {
  gsize lesson_count = 0;
  const LessonSpec *lessons = app_lessons_all(&lesson_count);
  GtkWidget *child = gtk_widget_get_first_child(self->list_box);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(self->list_box), child);
    child = next;
  }
  for (gint rank = 0; rank <= 4; ++rank) {
    for (gsize index = 0; index < lesson_count; ++index) {
      if (lesson_group_rank(lessons[index].difficulty) == rank) {
        add_lesson_row(self, &lessons[index]);
      }
    }
  }
  if (selected_node_id && *selected_node_id) {
    GtkWidget *row = gtk_widget_get_first_child(self->list_box);
    while (row) {
      const gchar *node_id = g_object_get_data(G_OBJECT(row), "node-id");
      if (g_strcmp0(node_id, selected_node_id) == 0) {
        gtk_list_box_select_row(GTK_LIST_BOX(self->list_box), GTK_LIST_BOX_ROW(row));
        return;
      }
      row = gtk_widget_get_next_sibling(row);
    }
  }
  gtk_list_box_select_row(GTK_LIST_BOX(self->list_box), gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->list_box), 0));
}

static void settings_sync_choice_label(MainWindow *self) {
  const gchar *lang = app_lang_normalize(self->pending_language ? self->pending_language : self->config->language);
  gtk_label_set_text(GTK_LABEL(self->settings_choice_label),
                     g_strcmp0(lang, "zh-CN") == 0
                       ? app_i18n_get(self->config->language, "wizard.language.zh_cn")
                       : app_i18n_get(self->config->language, "wizard.language.english"));
}

static void apply_language_to_ui(MainWindow *self) {
  const gchar *lang = app_lang_normalize(self->config->language);
  gtk_window_set_title(GTK_WINDOW(self->window), app_i18n_get(self->config->language, "list1.window_title"));
  gtk_label_set_text(GTK_LABEL(self->header_title_label), app_i18n_get(self->config->language, "list1.window_title"));
  gtk_label_set_text(GTK_LABEL(self->sidebar_title_label), app_i18n_get(self->config->language, "list1.lessons"));
  if (g_strcmp0(lang, "zh-CN") == 0) {
    gtk_widget_add_css_class(self->root, "lang-zh");
  } else {
    gtk_widget_remove_css_class(self->root, "lang-zh");
  }
  update_titlebar_tooltips(self);
  populate_lesson_list(self, self->current_node_id);
  if (self->current_lesson) {
    update_for_lesson(self, app_lesson_find(self->current_node_id));
  }
  if (self->quiz_title_label) {
    gtk_label_set_text(GTK_LABEL(self->quiz_title_label), app_i18n_get(self->config->language, "quiz.title"));
  }
  if (self->quiz_intro_label) {
    gtk_label_set_text(GTK_LABEL(self->quiz_intro_label), app_i18n_get(self->config->language, "quiz.subtitle"));
  }
  if (self->quiz_submit_button) {
    gtk_button_set_label(GTK_BUTTON(self->quiz_submit_button), app_i18n_get(self->config->language, "quiz.submit"));
  }
  if (self->quiz_result_label) {
    gtk_label_set_text(GTK_LABEL(self->quiz_result_label), app_i18n_get(self->config->language, "quiz.ready"));
  }
  if (self->quiz_questions_box) {
    rebuild_quiz_questions(self);
  }
  if (self->settings_window) {
    gtk_window_set_title(GTK_WINDOW(self->settings_window), app_i18n_get(self->config->language, "settings.window_title"));
    gtk_label_set_text(GTK_LABEL(self->settings_title_label), app_i18n_get(self->config->language, "settings.window_title"));
    gtk_label_set_text(GTK_LABEL(self->settings_language_label), app_i18n_get(self->config->language, "settings.system_language"));
    gtk_label_set_text(GTK_LABEL(self->settings_select_label), app_i18n_get(self->config->language, "settings.select_language"));
    gtk_button_set_label(GTK_BUTTON(self->settings_lang_zh_button), app_i18n_get(self->config->language, "wizard.language.zh_cn"));
    gtk_button_set_label(GTK_BUTTON(self->settings_lang_en_button), app_i18n_get(self->config->language, "wizard.language.english"));
    gtk_button_set_label(GTK_BUTTON(self->settings_save_button), app_i18n_get(self->config->language, "settings.save"));
    settings_sync_choice_label(self);
    update_titlebar_tooltips(self);
  }
}

static void persist_language_change(MainWindow *self) {
  g_autoptr(GError) error = NULL;
  app_db_set_string(self->db, "language", app_lang_normalize(self->config->language), &error);
  app_config_save(self->config, &error);
}

static void on_settings_save_clicked(GtkButton *button, gpointer user_data) {
  MainWindow *self = user_data;
  (void)button;
  app_config_set_language(self->config, self->pending_language ? self->pending_language : self->config->language);
  persist_language_change(self);
  apply_language_to_ui(self);
  show_banner(self, "banner-success", app_i18n_get(self->config->language, "settings.saved"));
}

static void on_settings_pick_zh(GtkButton *button, gpointer user_data) {
  MainWindow *self = user_data;
  (void)button;
  g_free(self->pending_language);
  self->pending_language = g_strdup("zh-CN");
  settings_sync_choice_label(self);
}

static void on_settings_pick_en(GtkButton *button, gpointer user_data) {
  MainWindow *self = user_data;
  (void)button;
  g_free(self->pending_language);
  self->pending_language = g_strdup("en");
  settings_sync_choice_label(self);
}

static void on_settings_toggle_choices(GtkButton *button, gpointer user_data) {
  MainWindow *self = user_data;
  gboolean reveal = !gtk_revealer_get_reveal_child(GTK_REVEALER(self->settings_revealer));
  (void)button;
  gtk_revealer_set_reveal_child(GTK_REVEALER(self->settings_revealer), reveal);
}

static GtkWidget *create_settings_headerbar(MainWindow *self, GtkWidget *settings_window) {
  GtkWidget *header = gtk_header_bar_new();
  GtkWidget *close_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(header, "app-headerbar");
  self->settings_title_label = gtk_label_new(app_i18n_get(self->config->language, "settings.window_title"));
  gtk_widget_add_css_class(self->settings_title_label, "titlebar-title");
  gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header), FALSE);
  gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), self->settings_title_label);
  self->settings_close_button = create_titlebar_dot_button("close.svg", app_i18n_get(self->config->language, "settings.close_panel"), G_CALLBACK(on_titlebar_close_clicked), settings_window);
  gtk_box_append(GTK_BOX(close_box), self->settings_close_button);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), close_box);
  return header;
}

static void on_settings_window_destroy(GtkWidget *widget, gpointer user_data) {
  MainWindow *self = user_data;
  (void)widget;
  self->settings_window = NULL;
  self->settings_title_label = NULL;
  self->settings_language_label = NULL;
  self->settings_select_button = NULL;
  self->settings_select_label = NULL;
  self->settings_choice_label = NULL;
  self->settings_revealer = NULL;
  self->settings_lang_zh_button = NULL;
  self->settings_lang_en_button = NULL;
  self->settings_save_button = NULL;
  self->settings_close_button = NULL;
}

static void present_settings_window(MainWindow *self) {
  if (!self->settings_window) {
    GtkWidget *window = gtk_application_window_new(self->app);
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *label_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *choice_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *save_button = gtk_button_new_with_label(app_i18n_get(self->config->language, "settings.save"));
    GtkWidget *select_button = gtk_button_new();
    GtkWidget *select_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *select_label = gtk_label_new(app_i18n_get(self->config->language, "settings.select_language"));
    GtkWidget *choice_label = gtk_label_new(NULL);
    GtkWidget *lang_label = gtk_label_new(app_i18n_get(self->config->language, "settings.system_language"));
    GtkWidget *revealer = gtk_revealer_new();
    GtkWidget *reveal_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *zh_button = gtk_button_new_with_label(app_i18n_get(self->config->language, "wizard.language.zh_cn"));
    GtkWidget *en_button = gtk_button_new_with_label(app_i18n_get(self->config->language, "wizard.language.english"));
    self->settings_window = window;
    self->settings_language_label = lang_label;
    self->settings_select_button = select_button;
    self->settings_select_label = select_label;
    self->settings_choice_label = choice_label;
    self->settings_revealer = revealer;
    self->settings_lang_zh_button = zh_button;
    self->settings_lang_en_button = en_button;
    self->settings_save_button = save_button;
    gtk_window_set_title(GTK_WINDOW(window), app_i18n_get(self->config->language, "settings.window_title"));
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(self->window));
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(window), 430, 240);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_titlebar(GTK_WINDOW(window), create_settings_headerbar(self, window));
    gtk_widget_add_css_class(root, "app-shell");
    gtk_widget_add_css_class(root, "settings-shell");
    gtk_widget_add_css_class(row, "settings-row");
    gtk_widget_add_css_class(save_button, "accent-button");
    gtk_widget_add_css_class(select_button, "settings-select-button");
    gtk_widget_add_css_class(lang_label, "section-title");
    gtk_widget_add_css_class(select_label, "muted-label");
    gtk_widget_add_css_class(choice_label, "value-label");
    gtk_label_set_xalign(GTK_LABEL(lang_label), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(select_label), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(choice_label), 0.0f);
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 220);
    gtk_box_append(GTK_BOX(select_content), select_label);
    gtk_box_append(GTK_BOX(select_content), choice_label);
    gtk_button_set_child(GTK_BUTTON(select_button), select_content);
    gtk_box_append(GTK_BOX(reveal_box), zh_button);
    gtk_box_append(GTK_BOX(reveal_box), en_button);
    gtk_revealer_set_child(GTK_REVEALER(revealer), reveal_box);
    gtk_box_append(GTK_BOX(label_box), lang_label);
    gtk_box_append(GTK_BOX(choice_box), select_button);
    gtk_box_append(GTK_BOX(choice_box), revealer);
    gtk_box_append(GTK_BOX(row), label_box);
    gtk_box_append(GTK_BOX(row), choice_box);
    gtk_box_append(GTK_BOX(buttons), save_button);
    gtk_box_append(GTK_BOX(root), row);
    gtk_box_append(GTK_BOX(root), buttons);
    gtk_window_set_child(GTK_WINDOW(window), root);
    g_signal_connect(select_button, "clicked", G_CALLBACK(on_settings_toggle_choices), self);
    g_signal_connect(zh_button, "clicked", G_CALLBACK(on_settings_pick_zh), self);
    g_signal_connect(en_button, "clicked", G_CALLBACK(on_settings_pick_en), self);
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_settings_save_clicked), self);
    g_signal_connect(window, "destroy", G_CALLBACK(on_settings_window_destroy), self);
  }
  g_free(self->pending_language);
  self->pending_language = g_strdup(app_lang_normalize(self->config->language));
  settings_sync_choice_label(self);
  gtk_revealer_set_reveal_child(GTK_REVEALER(self->settings_revealer), FALSE);
  gtk_window_present(GTK_WINDOW(self->settings_window));
}

static void on_titlebar_settings_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  present_settings_window(user_data);
}

static GtkWidget *create_custom_headerbar(MainWindow *self) {
  GtkWidget *header = gtk_header_bar_new();
  GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *left_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *settings_button = gtk_button_new();
  GtkWidget *settings_image = gtk_image_new_from_file(self->gear_path);
  self->header_title_label = gtk_label_new(app_i18n_get(self->config->language, "list1.window_title"));
  gtk_widget_add_css_class(header, "app-headerbar");
  gtk_widget_add_css_class(controls, "titlebar-dots");
  gtk_widget_add_css_class(left_controls, "titlebar-left");
  gtk_widget_add_css_class(self->header_title_label, "titlebar-title");
  gtk_widget_add_css_class(settings_button, "titlebar-gear-button");
  gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header), FALSE);
  gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), self->header_title_label);
  gtk_button_set_child(GTK_BUTTON(settings_button), settings_image);
  self->titlebar_settings_button = settings_button;
  self->titlebar_min_button = create_titlebar_dot_button("minimize.svg", app_i18n_get(self->config->language, "titlebar.minimize"), G_CALLBACK(on_titlebar_minimize_clicked), self->window);
  self->titlebar_max_button = create_titlebar_dot_button("maximize.svg", app_i18n_get(self->config->language, "titlebar.maximize"), G_CALLBACK(on_titlebar_maximize_clicked), self->window);
  self->titlebar_close_button = create_titlebar_dot_button("close.svg", app_i18n_get(self->config->language, "titlebar.close"), G_CALLBACK(on_titlebar_close_clicked), self->window);
  gtk_widget_set_tooltip_text(settings_button, app_i18n_get(self->config->language, "titlebar.settings"));
  g_signal_connect(settings_button, "clicked", G_CALLBACK(on_titlebar_settings_clicked), self);
  gtk_box_append(GTK_BOX(left_controls), settings_button);
  gtk_box_append(GTK_BOX(controls), self->titlebar_min_button);
  gtk_box_append(GTK_BOX(controls), self->titlebar_max_button);
  gtk_box_append(GTK_BOX(controls), self->titlebar_close_button);
  gtk_header_bar_pack_start(GTK_HEADER_BAR(header), left_controls);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), controls);
  return header;
}

static GtkWidget *create_sidebar(MainWindow *self) {
  GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  self->sidebar_title_label = create_section_label(app_i18n_get(self->config->language, "list1.lessons"));
  self->list_box = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->list_box), GTK_SELECTION_SINGLE);
  gtk_widget_add_css_class(card, "panel-card");
  gtk_widget_add_css_class(self->list_box, "lesson-list");
  gtk_widget_set_vexpand(card, TRUE);
  gtk_widget_set_vexpand(self->list_box, TRUE);
  gtk_box_append(GTK_BOX(card), self->sidebar_title_label);
  gtk_box_append(GTK_BOX(card), self->list_box);
  populate_lesson_list(self, NULL);
  g_signal_connect(self->list_box, "row-selected", G_CALLBACK(on_row_selected), self);
  return card;
}

static GtkWidget *create_example_card(MainWindow *self) {
  GtkWidget *card = create_fold_card(self, &self->example_card, app_i18n_get(self->config->language, "module.example_code"));
  GtkWidget *hint = gtk_label_new(app_i18n_get(self->config->language, "module.code_hint"));
  GtkWidget *scroller = gtk_scrolled_window_new();
  GtkWidget *view;
  gtk_widget_add_css_class(hint, "muted-label");
  gtk_label_set_xalign(GTK_LABEL(hint), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  self->example_buffer = gtk_source_buffer_new(NULL);
  gtk_source_buffer_set_highlight_syntax(self->example_buffer, FALSE);
  view = gtk_source_view_new_with_buffer(self->example_buffer);
  self->example_view = view;
  gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(view), FALSE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 12);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(view), 12);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 12);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 12);
  gtk_widget_add_css_class(view, "code-surface");
  gtk_widget_set_size_request(scroller, -1, 190);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), view);
  gtk_box_append(GTK_BOX(self->example_card.content_box), hint);
  gtk_box_append(GTK_BOX(self->example_card.content_box), scroller);
  return card;
}

static GtkWidget *create_terminal_card(MainWindow *self) {
  GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget *scroller = gtk_scrolled_window_new();
  GtkWidget *terminal_view = gtk_text_view_new();
  self->terminal_title_label = gtk_label_new(app_i18n_get(self->config->language, "list1.terminal_title"));
  self->terminal_empty_label = gtk_label_new(app_i18n_get(self->config->language, "list1.terminal_empty"));
  self->terminal_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(terminal_view));
  gtk_widget_add_css_class(card, "panel-card");
  gtk_widget_add_css_class(card, "terminal-card");
  gtk_widget_add_css_class(self->terminal_title_label, "section-title");
  gtk_widget_add_css_class(self->terminal_empty_label, "terminal-empty-label");
  gtk_widget_add_css_class(terminal_view, "terminal-surface");
  gtk_label_set_xalign(GTK_LABEL(self->terminal_title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(self->terminal_empty_label), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(self->terminal_empty_label), TRUE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(terminal_view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(terminal_view), FALSE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(terminal_view), TRUE);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(terminal_view), 12);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(terminal_view), 12);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(terminal_view), 12);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(terminal_view), 12);
  gtk_widget_set_vexpand(scroller, TRUE);
  gtk_widget_set_size_request(scroller, -1, 220);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), terminal_view);
  gtk_box_append(GTK_BOX(card), self->terminal_title_label);
  gtk_box_append(GTK_BOX(card), self->terminal_empty_label);
  gtk_box_append(GTK_BOX(card), scroller);
  terminal_set_text(self, "");
  self->terminal_card = card;
  return card;
}

static GtkWidget *create_center_panel(MainWindow *self) {
  GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  GtkWidget *page_header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  GtkWidget *meta_card = create_fold_card(self, &self->meta_card, app_i18n_get(self->config->language, "module.current_node"));
  GtkWidget *headers_card = create_fold_card(self, &self->headers_card, app_i18n_get(self->config->language, "module.headers"));
  GtkWidget *goal_card = create_fold_card(self, &self->goal_card, app_i18n_get(self->config->language, "module.goal"));
  self->page_title_label = gtk_label_new(NULL);
  self->page_subtitle_label = gtk_label_new(NULL);
  self->headers_body_label = gtk_label_new(NULL);
  self->goal_body_label = gtk_label_new(NULL);
  gtk_widget_add_css_class(panel, "content-column");
  gtk_widget_add_css_class(page_header, "hero-card");
  gtk_widget_add_css_class(self->page_title_label, "hero-title");
  gtk_widget_add_css_class(self->page_subtitle_label, "hero-subtitle");
  gtk_widget_add_css_class(self->headers_body_label, "content-body");
  gtk_widget_add_css_class(self->goal_body_label, "content-body");
  gtk_label_set_xalign(GTK_LABEL(self->page_title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(self->page_subtitle_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(self->headers_body_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(self->goal_body_label), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(self->page_subtitle_label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(self->headers_body_label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(self->goal_body_label), TRUE);
  gtk_box_append(GTK_BOX(page_header), self->page_title_label);
  gtk_box_append(GTK_BOX(page_header), self->page_subtitle_label);
  gtk_box_append(GTK_BOX(panel), page_header);
  gtk_box_append(GTK_BOX(panel), create_quiz_card(self));
  gtk_box_append(GTK_BOX(panel), create_fold_card(self, &self->notes_card, app_i18n_get(self->config->language, "module.lesson_notes")));
  gtk_box_append(GTK_BOX(self->meta_card.content_box), create_value_row(app_i18n_get(self->config->language, "list1.current_node"), &self->current_node_value));
  gtk_box_append(GTK_BOX(self->meta_card.content_box), create_value_row(app_i18n_get(self->config->language, "list1.database_path"), &self->database_value));
  gtk_box_append(GTK_BOX(self->meta_card.content_box), create_value_row(app_i18n_get(self->config->language, "list1.language"), &self->language_value));
  gtk_box_append(GTK_BOX(self->meta_card.content_box), create_value_row(app_i18n_get(self->config->language, "list1.compiler_root"), &self->compiler_root_value));
  gtk_box_append(GTK_BOX(panel), meta_card);
  gtk_box_append(GTK_BOX(self->headers_card.content_box), self->headers_body_label);
  gtk_box_append(GTK_BOX(panel), headers_card);
  gtk_box_append(GTK_BOX(panel), create_example_card(self));
  gtk_box_append(GTK_BOX(self->goal_card.content_box), self->goal_body_label);
  gtk_box_append(GTK_BOX(panel), goal_card);
  gtk_box_append(GTK_BOX(panel), create_terminal_card(self));
  return panel;
}

static GtkWidget *create_editor_panel(MainWindow *self) {
  GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  GtkWidget *editor_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget *header_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *header_text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  GtkWidget *scroller = gtk_scrolled_window_new();
  GtkWidget *source_view;
  GtkWidget *run_icon;
  GtkWidget *run_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  g_autofree gchar *run_icon_path = g_build_filename(app_paths_get_data_dir(), "ui-icons", "run.svg", NULL);
  self->editor_title_label = gtk_label_new(NULL);
  self->editor_subtitle_label = gtk_label_new(NULL);
  self->status_label = gtk_label_new(NULL);
  self->run_button = gtk_button_new();
  self->run_button_label = gtk_label_new(NULL);
  self->source_buffer = gtk_source_buffer_new(NULL);
  gtk_source_buffer_set_highlight_syntax(self->source_buffer, FALSE);
  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->source_buffer), machine_demo_code(), -1);
  g_signal_connect(self->source_buffer, "changed", G_CALLBACK(on_source_buffer_changed), self);
  source_view = gtk_source_view_new_with_buffer(self->source_buffer);
  self->source_view = source_view;
  gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(source_view), TRUE);
  gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(source_view), 2);
  gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(source_view), FALSE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(source_view), TRUE);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(source_view), 12);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(source_view), 12);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(source_view), 12);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(source_view), 12);
  gtk_widget_add_css_class(source_view, "code-surface");
  run_icon = gtk_image_new_from_file(run_icon_path);
  gtk_box_append(GTK_BOX(run_content), run_icon);
  gtk_box_append(GTK_BOX(run_content), self->run_button_label);
  gtk_button_set_child(GTK_BUTTON(self->run_button), run_content);
  gtk_widget_add_css_class(self->run_button, "accent-button");
  gtk_widget_add_css_class(self->run_button, "run-button");
  gtk_widget_set_halign(self->run_button, GTK_ALIGN_END);
  g_signal_connect(self->run_button, "clicked", G_CALLBACK(on_run_clicked), self);
  gtk_widget_add_css_class(panel, "editor-column");
  gtk_widget_add_css_class(editor_card, "panel-card");
  gtk_widget_add_css_class(self->editor_title_label, "section-title");
  gtk_widget_add_css_class(self->editor_subtitle_label, "muted-label");
  gtk_widget_add_css_class(self->status_label, "status-pill");
  gtk_widget_add_css_class(header_row, "editor-header-row");
  gtk_label_set_xalign(GTK_LABEL(self->editor_title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(self->editor_subtitle_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(self->status_label), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(self->editor_subtitle_label), TRUE);
  gtk_widget_set_hexpand(header_text, TRUE);
  gtk_widget_set_hexpand(scroller, TRUE);
  gtk_widget_set_vexpand(scroller, TRUE);
  gtk_box_append(GTK_BOX(header_text), self->editor_title_label);
  gtk_box_append(GTK_BOX(header_text), self->editor_subtitle_label);
  gtk_box_append(GTK_BOX(header_row), header_text);
  gtk_box_append(GTK_BOX(header_row), self->run_button);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), source_view);
  gtk_box_append(GTK_BOX(editor_card), header_row);
  gtk_box_append(GTK_BOX(editor_card), scroller);
  gtk_box_append(GTK_BOX(editor_card), self->status_label);
  gtk_box_append(GTK_BOX(panel), editor_card);
  self->editor_panel = panel;
  return panel;
}

MainWindow *main_window_new(GtkApplication *app, AppConfig *config, AppDatabase *db) {
  MainWindow *self = g_new0(MainWindow, 1);
  GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
  GtkWidget *sidebar_scroller = gtk_scrolled_window_new();
  GtkWidget *center_scroller = gtk_scrolled_window_new();
  GtkWidget *sidebar;
  GtkWidget *center_panel;
  GtkWidget *editor_panel;
  GtkWidget *header;
  GtkWidget *banner_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  g_autofree gchar *saved_node = NULL;
  self->app = app;
  self->config = config;
  self->db = db;
  for (gsize qi = 0; qi < G_N_ELEMENTS(self->quiz_answers); ++qi) {
    self->quiz_answers[qi] = -1;
  }
  self->window = gtk_application_window_new(app);
  self->chevron_up_path = g_build_filename(app_paths_get_data_dir(), "ui-icons", "chevron-up.svg", NULL);
  self->chevron_down_path = g_build_filename(app_paths_get_data_dir(), "ui-icons", "chevron-down.svg", NULL);
  self->gear_path = g_build_filename(app_paths_get_data_dir(), "ui-icons", "gear.svg", NULL);
  gtk_window_set_title(GTK_WINDOW(self->window), app_i18n_get(config->language, "list1.window_title"));
  gtk_window_set_default_size(GTK_WINDOW(self->window), 1120, 680);
  gtk_window_set_resizable(GTK_WINDOW(self->window), TRUE);
  self->root = root;
  gtk_widget_add_css_class(root, "app-shell");
  if (g_strcmp0(app_lang_normalize(config->language), "zh-CN") == 0) {
    gtk_widget_add_css_class(root, "lang-zh");
  }
  gtk_widget_add_css_class(content, "app-content");
  gtk_widget_set_hexpand(content, TRUE);
  gtk_widget_set_vexpand(content, TRUE);
  self->banner_revealer = gtk_revealer_new();
  self->banner_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  self->banner_label = gtk_label_new(NULL);
  gtk_widget_add_css_class(self->banner_box, "banner-box");
  gtk_widget_add_css_class(self->banner_label, "banner-label");
  gtk_label_set_xalign(GTK_LABEL(self->banner_label), 0.0f);
  gtk_box_append(GTK_BOX(banner_content), self->banner_label);
  gtk_box_append(GTK_BOX(self->banner_box), banner_content);
  gtk_revealer_set_transition_type(GTK_REVEALER(self->banner_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
  gtk_revealer_set_transition_duration(GTK_REVEALER(self->banner_revealer), 280);
  gtk_revealer_set_child(GTK_REVEALER(self->banner_revealer), self->banner_box);
  header = create_custom_headerbar(self);
  gtk_window_set_titlebar(GTK_WINDOW(self->window), header);
  sidebar = create_sidebar(self);
  center_panel = create_center_panel(self);
  editor_panel = create_editor_panel(self);
  gtk_widget_set_size_request(sidebar_scroller, 290, -1);
  gtk_widget_set_hexpand(center_panel, TRUE);
  gtk_widget_set_hexpand(editor_panel, TRUE);
  gtk_widget_set_vexpand(editor_panel, TRUE);
  gtk_widget_set_vexpand(sidebar_scroller, TRUE);
  gtk_widget_set_vexpand(center_scroller, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(center_scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(sidebar_scroller), FALSE);
  gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(center_scroller), FALSE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sidebar_scroller), sidebar);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(center_scroller), center_panel);
  gtk_box_append(GTK_BOX(content), sidebar_scroller);
  gtk_box_append(GTK_BOX(content), center_scroller);
  gtk_box_append(GTK_BOX(content), editor_panel);
  gtk_box_append(GTK_BOX(root), self->banner_revealer);
  gtk_box_append(GTK_BOX(root), content);
  gtk_window_set_child(GTK_WINDOW(self->window), root);
  saved_node = app_db_get_string(self->db, "current_node");
  populate_lesson_list(self, saved_node ? saved_node : "overview.all");
  g_signal_connect(self->window, "close-request", G_CALLBACK(on_window_close), self);
  g_signal_connect(self->window, "destroy", G_CALLBACK(on_window_destroy), self);
  return self;
}

void main_window_present(MainWindow *window) {
  gtk_window_present(GTK_WINDOW(window->window));
}

void main_window_destroy(MainWindow *window) {
  if (!window) {
    return;
  }
  flush_editor_save(window);
  if (window->banner_timeout_id != 0) {
    g_source_remove(window->banner_timeout_id);
  }
  if (GTK_IS_WINDOW(window->window)) {
    gtk_window_destroy(GTK_WINDOW(window->window));
  }
  g_clear_pointer(&window->current_node_id, g_free);
  g_clear_pointer(&window->chevron_up_path, g_free);
  g_clear_pointer(&window->chevron_down_path, g_free);
  g_clear_pointer(&window->gear_path, g_free);
  g_clear_pointer(&window->pending_language, g_free);
  g_clear_object(&window->example_buffer);
  g_clear_object(&window->source_buffer);
  g_free(window);
}
