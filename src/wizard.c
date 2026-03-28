#include "wizard.h"

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "config.h"
#include "db.h"
#include "i18n.h"

struct WizardWindow {
  GtkApplication *app;
  GtkWidget *window;
  GtkWidget *stack;
  GtkWidget *title_label;
  GtkWidget *subtitle_label;
  GtkWidget *language_title_label;
  GtkWidget *language_body_label;
  GtkWidget *database_title_label;
  GtkWidget *database_body_label;
  GtkWidget *database_note_label;
  GtkWidget *path_title_label;
  GtkWidget *path_value_label;
  GtkWidget *english_button;
  GtkWidget *zh_cn_button;
  GtkWidget *back_button;
  GtkWidget *next_button;
  GtkWidget *browse_button;
  GtkWidget *finish_button;
  AppConfig *config;
  WizardCompleteCallback callback;
  gpointer user_data;
};

static void wizard_error_close_clicked(GtkButton *button, gpointer user_data) {
  GtkWindow *window = GTK_WINDOW(user_data);
  (void)button;
  gtk_window_destroy(window);
}

static void show_error_dialog(GtkWindow *parent, const gchar *message) {
  GtkWidget *window = gtk_window_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  GtkWidget *title = gtk_label_new("Error");
  GtkWidget *body = gtk_label_new(message ? message : "Unknown error.");
  GtkWidget *button = gtk_button_new_with_label("Close");

  gtk_window_set_title(GTK_WINDOW(window), "Error");
  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(window), parent);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_window_set_default_size(GTK_WINDOW(window), 480, 180);
  gtk_widget_set_margin_top(box, 20);
  gtk_widget_set_margin_bottom(box, 20);
  gtk_widget_set_margin_start(box, 20);
  gtk_widget_set_margin_end(box, 20);
  gtk_widget_add_css_class(title, "section-title");
  gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(body), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(body), TRUE);
  gtk_widget_set_halign(button, GTK_ALIGN_END);

  gtk_box_append(GTK_BOX(box), title);
  gtk_box_append(GTK_BOX(box), body);
  gtk_box_append(GTK_BOX(box), button);
  gtk_window_set_child(GTK_WINDOW(window), box);
  g_signal_connect(button, "clicked", G_CALLBACK(wizard_error_close_clicked), window);
  gtk_window_present(GTK_WINDOW(window));
}


static void wizard_update_navigation(WizardWindow *wizard) {
  const gchar *page = gtk_stack_get_visible_child_name(GTK_STACK(wizard->stack));
  const gboolean on_database_page = g_strcmp0(page, "database") == 0;

  gtk_widget_set_sensitive(wizard->back_button, on_database_page);
  gtk_widget_set_visible(wizard->next_button, !on_database_page);
}

static void on_wizard_destroy(GtkWidget *widget, gpointer user_data) {
  WizardWindow *wizard = user_data;
  (void)widget;
  wizard->window = NULL;
}

static void wizard_update_text(WizardWindow *wizard) {
  gtk_window_set_title(GTK_WINDOW(wizard->window), app_i18n_get(wizard->config->language, "wizard.window_title"));
  gtk_label_set_text(GTK_LABEL(wizard->title_label), app_i18n_get(wizard->config->language, "wizard.title"));
  gtk_label_set_text(GTK_LABEL(wizard->subtitle_label), app_i18n_get(wizard->config->language, "wizard.subtitle"));
  gtk_label_set_text(GTK_LABEL(wizard->language_title_label), app_i18n_get(wizard->config->language, "wizard.language.title"));
  gtk_label_set_text(GTK_LABEL(wizard->language_body_label), app_i18n_get(wizard->config->language, "wizard.language.body"));
  gtk_label_set_text(GTK_LABEL(wizard->database_title_label), app_i18n_get(wizard->config->language, "wizard.database.title"));
  gtk_label_set_text(GTK_LABEL(wizard->database_body_label), app_i18n_get(wizard->config->language, "wizard.database.body"));
  gtk_label_set_text(GTK_LABEL(wizard->database_note_label), app_i18n_get(wizard->config->language, "wizard.database.note"));
  gtk_label_set_text(GTK_LABEL(wizard->path_title_label), app_i18n_get(wizard->config->language, "wizard.database.selected"));
  gtk_button_set_label(GTK_BUTTON(wizard->english_button), app_i18n_get(wizard->config->language, "wizard.language.english"));
  gtk_button_set_label(GTK_BUTTON(wizard->zh_cn_button), app_i18n_get(wizard->config->language, "wizard.language.zh_cn"));
  gtk_button_set_label(GTK_BUTTON(wizard->back_button), app_i18n_get(wizard->config->language, "wizard.back"));
  gtk_button_set_label(GTK_BUTTON(wizard->next_button), app_i18n_get(wizard->config->language, "wizard.next"));
  gtk_button_set_label(GTK_BUTTON(wizard->browse_button), app_i18n_get(wizard->config->language, "wizard.database.browse"));
  gtk_button_set_label(GTK_BUTTON(wizard->finish_button), app_i18n_get(wizard->config->language, "wizard.database.confirm"));

  if (wizard->config->db_dir) {
    gtk_label_set_text(GTK_LABEL(wizard->path_value_label), wizard->config->db_dir);
  } else {
    gtk_label_set_text(GTK_LABEL(wizard->path_value_label), app_i18n_get(wizard->config->language, "wizard.database.none"));
  }

  wizard_update_navigation(wizard);
}

static void on_choose_language(GtkButton *button, gpointer user_data) {
  WizardWindow *wizard = user_data;
  const gchar *language = g_object_get_data(G_OBJECT(button), "language");

  app_config_set_language(wizard->config, language);
  wizard_update_text(wizard);
}

static void on_back_clicked(GtkButton *button, gpointer user_data) {
  WizardWindow *wizard = user_data;
  (void)button;
  gtk_stack_set_visible_child_name(GTK_STACK(wizard->stack), "language");
  wizard_update_navigation(wizard);
}

static void on_next_clicked(GtkButton *button, gpointer user_data) {
  WizardWindow *wizard = user_data;
  (void)button;
  gtk_stack_set_visible_child_name(GTK_STACK(wizard->stack), "database");
  wizard_update_navigation(wizard);
}

#if GTK_CHECK_VERSION(4, 10, 0)
static void on_folder_selected(GObject *source_object, GAsyncResult *result, gpointer user_data) {
  WizardWindow *wizard = user_data;
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
  g_autoptr(GError) error = NULL;
  g_autoptr(GFile) file = NULL;
  g_autofree gchar *path = NULL;

  file = gtk_file_dialog_select_folder_finish(dialog, result, &error);
  if (error) {
    if (!g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED)) {
      show_error_dialog(GTK_WINDOW(wizard->window), error->message);
    }
    wizard_update_text(wizard);
    return;
  }

  path = file ? g_file_get_path(file) : NULL;
  if (path) {
    app_config_set_db_dir(wizard->config, path);
  }

  wizard_update_text(wizard);
}
#else
static void on_folder_dialog_response(GtkNativeDialog *dialog, gint response, gpointer user_data) {
  WizardWindow *wizard = user_data;

  if (response == GTK_RESPONSE_ACCEPT) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    g_autoptr(GFile) file = gtk_file_chooser_get_file(chooser);
    g_autofree gchar *path = file ? g_file_get_path(file) : NULL;

    if (path) {
      app_config_set_db_dir(wizard->config, path);
    }
  }

  wizard_update_text(wizard);
  g_object_unref(dialog);
}
#endif

static void on_browse_clicked(GtkButton *button, gpointer user_data) {
  WizardWindow *wizard = user_data;

  (void)button;

#if GTK_CHECK_VERSION(4, 10, 0)
  {
    GtkFileDialog *dialog = gtk_file_dialog_new();

    gtk_file_dialog_set_title(dialog, app_i18n_get(wizard->config->language, "wizard.choose_folder_title"));
    gtk_file_dialog_set_modal(dialog, TRUE);
    gtk_file_dialog_set_accept_label(dialog, app_i18n_get(wizard->config->language, "wizard.database.browse"));

    if (wizard->config->db_dir) {
      g_autoptr(GFile) initial_folder = g_file_new_for_path(wizard->config->db_dir);
      gtk_file_dialog_set_initial_folder(dialog, initial_folder);
    }

    gtk_file_dialog_select_folder(dialog,
                                  GTK_WINDOW(wizard->window),
                                  NULL,
                                  on_folder_selected,
                                  wizard);
    g_object_unref(dialog);
  }
#else
  {
    GtkFileChooserNative *dialog = gtk_file_chooser_native_new(app_i18n_get(wizard->config->language, "wizard.choose_folder_title"),
                                                               GTK_WINDOW(wizard->window),
                                                               GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                               app_i18n_get(wizard->config->language, "wizard.database.browse"),
                                                               app_i18n_get(wizard->config->language, "wizard.back"));
    g_signal_connect(dialog, "response", G_CALLBACK(on_folder_dialog_response), wizard);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
  }
#endif
}

static gboolean bootstrap_database(WizardWindow *wizard, GError **error) {
  AppDatabase db = {0};
  gboolean ok = FALSE;

  if (!wizard->config->db_dir) {
    g_set_error(error,
                g_quark_from_static_string("ml-wizard"),
                1,
                "%s",
                app_i18n_get(wizard->config->language, "wizard.database.empty"));
    return FALSE;
  }

  if (g_mkdir_with_parents(wizard->config->db_dir, 0700) != 0) {
    g_set_error(error,
                g_quark_from_static_string("ml-wizard"),
                2,
                "Failed to create the selected directory.");
    return FALSE;
  }

  if (!app_db_open(&db, wizard->config->db_path, error)) {
    return FALSE;
  }

  if (!app_db_init_schema(&db, error)) {
    goto cleanup;
  }

  if (!app_db_set_string(&db, "language", app_lang_normalize(wizard->config->language), error)) {
    goto cleanup;
  }

  if (!app_db_set_string(&db, "current_node", "intro.overview", error)) {
    goto cleanup;
  }

  wizard->config->setup_complete = TRUE;
  if (!app_config_save(wizard->config, error)) {
    goto cleanup;
  }

  ok = TRUE;

cleanup:
  app_db_close(&db);
  return ok;
}

static void on_finish_clicked(GtkButton *button, gpointer user_data) {
  WizardWindow *wizard = user_data;
  g_autoptr(GError) error = NULL;

  (void)button;

  if (!bootstrap_database(wizard, &error)) {
    show_error_dialog(GTK_WINDOW(wizard->window), error ? error->message : app_i18n_get(wizard->config->language, "wizard.setup_failed"));
    return;
  }

  if (wizard->callback) {
    wizard->callback(wizard->user_data);
  }
}

WizardWindow *wizard_window_new(GtkApplication *app,
                                AppConfig *config,
                                WizardCompleteCallback callback,
                                gpointer user_data) {
  WizardWindow *wizard = g_new0(WizardWindow, 1);
  GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
  GtkWidget *stack = gtk_stack_new();
  GtkWidget *language_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
  GtkWidget *database_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
  GtkWidget *language_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  wizard->app = app;
  wizard->config = config;
  wizard->callback = callback;
  wizard->user_data = user_data;
  wizard->window = gtk_application_window_new(app);
  wizard->stack = stack;

  gtk_window_set_default_size(GTK_WINDOW(wizard->window), 1000, 700);
  gtk_window_set_resizable(GTK_WINDOW(wizard->window), FALSE);
  gtk_window_set_title(GTK_WINDOW(wizard->window), app_i18n_get(config->language, "wizard.window_title"));
  gtk_widget_add_css_class(root, "wizard-shell");
  gtk_widget_set_margin_top(root, 28);
  gtk_widget_set_margin_bottom(root, 28);
  gtk_widget_set_margin_start(root, 28);
  gtk_widget_set_margin_end(root, 28);

  wizard->title_label = gtk_label_new(NULL);
  wizard->subtitle_label = gtk_label_new(NULL);
  wizard->language_title_label = gtk_label_new(NULL);
  wizard->language_body_label = gtk_label_new(NULL);
  wizard->database_title_label = gtk_label_new(NULL);
  wizard->database_body_label = gtk_label_new(NULL);
  wizard->database_note_label = gtk_label_new(NULL);
  wizard->path_title_label = gtk_label_new(NULL);
  wizard->path_value_label = gtk_label_new(NULL);
  wizard->english_button = gtk_button_new();
  wizard->zh_cn_button = gtk_button_new();
  wizard->back_button = gtk_button_new();
  wizard->next_button = gtk_button_new();
  wizard->browse_button = gtk_button_new();
  wizard->finish_button = gtk_button_new();

  gtk_widget_add_css_class(wizard->title_label, "hero-title");
  gtk_widget_add_css_class(wizard->subtitle_label, "hero-subtitle");
  gtk_widget_add_css_class(wizard->language_title_label, "section-title");
  gtk_widget_add_css_class(wizard->database_title_label, "section-title");
  gtk_widget_add_css_class(wizard->language_body_label, "content-body");
  gtk_widget_add_css_class(wizard->database_body_label, "content-body");
  gtk_widget_add_css_class(wizard->database_note_label, "muted-label");
  gtk_widget_add_css_class(wizard->path_title_label, "muted-label");
  gtk_widget_add_css_class(wizard->path_value_label, "value-label");
  gtk_widget_add_css_class(wizard->english_button, "accent-button");
  gtk_widget_add_css_class(wizard->zh_cn_button, "accent-button");
  gtk_widget_add_css_class(wizard->finish_button, "accent-button");
  gtk_widget_add_css_class(language_page, "panel-card");
  gtk_widget_add_css_class(database_page, "panel-card");

  gtk_label_set_xalign(GTK_LABEL(wizard->title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(wizard->subtitle_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(wizard->language_title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(wizard->language_body_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(wizard->database_title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(wizard->database_body_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(wizard->database_note_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(wizard->path_title_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(wizard->path_value_label), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(wizard->subtitle_label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(wizard->language_body_label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(wizard->database_body_label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(wizard->database_note_label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(wizard->path_value_label), TRUE);

  g_object_set_data(G_OBJECT(wizard->english_button), "language", "en");
  g_object_set_data(G_OBJECT(wizard->zh_cn_button), "language", "zh-CN");
  g_signal_connect(wizard->english_button, "clicked", G_CALLBACK(on_choose_language), wizard);
  g_signal_connect(wizard->zh_cn_button, "clicked", G_CALLBACK(on_choose_language), wizard);
  g_signal_connect(wizard->back_button, "clicked", G_CALLBACK(on_back_clicked), wizard);
  g_signal_connect(wizard->next_button, "clicked", G_CALLBACK(on_next_clicked), wizard);
  g_signal_connect(wizard->browse_button, "clicked", G_CALLBACK(on_browse_clicked), wizard);
  g_signal_connect(wizard->finish_button, "clicked", G_CALLBACK(on_finish_clicked), wizard);

  gtk_box_append(GTK_BOX(language_buttons), wizard->english_button);
  gtk_box_append(GTK_BOX(language_buttons), wizard->zh_cn_button);

  gtk_box_append(GTK_BOX(language_page), wizard->language_title_label);
  gtk_box_append(GTK_BOX(language_page), wizard->language_body_label);
  gtk_box_append(GTK_BOX(language_page), language_buttons);

  gtk_box_append(GTK_BOX(database_page), wizard->database_title_label);
  gtk_box_append(GTK_BOX(database_page), wizard->database_body_label);
  gtk_box_append(GTK_BOX(database_page), wizard->database_note_label);
  gtk_box_append(GTK_BOX(database_page), wizard->path_title_label);
  gtk_box_append(GTK_BOX(database_page), wizard->path_value_label);
  gtk_box_append(GTK_BOX(database_page), wizard->browse_button);
  gtk_box_append(GTK_BOX(database_page), wizard->finish_button);

  gtk_stack_add_named(GTK_STACK(stack), language_page, "language");
  gtk_stack_add_named(GTK_STACK(stack), database_page, "database");
  gtk_stack_set_visible_child_name(GTK_STACK(stack), "language");

  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(footer), wizard->back_button);
  gtk_box_append(GTK_BOX(footer), spacer);
  gtk_box_append(GTK_BOX(footer), wizard->next_button);

  gtk_box_append(GTK_BOX(root), wizard->title_label);
  gtk_box_append(GTK_BOX(root), wizard->subtitle_label);
  gtk_box_append(GTK_BOX(root), stack);
  gtk_box_append(GTK_BOX(root), footer);
  gtk_window_set_child(GTK_WINDOW(wizard->window), root);
  g_signal_connect(wizard->window, "destroy", G_CALLBACK(on_wizard_destroy), wizard);

  wizard_update_text(wizard);
  return wizard;
}

void wizard_window_present(WizardWindow *wizard) {
  gtk_window_present(GTK_WINDOW(wizard->window));
}

void wizard_window_destroy(WizardWindow *wizard) {
  if (!wizard) {
    return;
  }

  if (GTK_IS_WINDOW(wizard->window)) {
    gtk_window_destroy(GTK_WINDOW(wizard->window));
  }

  g_free(wizard);
}
