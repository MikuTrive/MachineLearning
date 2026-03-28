#include <gtk/gtk.h>

#include "config.h"
#include "db.h"
#include "i18n.h"
#include "main_window.h"
#include "style.h"
#include "wizard.h"
#include "app_meta.h"

typedef struct {
  GtkApplication *app;
  AppConfig config;
  AppDatabase db;
  MainWindow *main_window;
  WizardWindow *wizard_window;
  gboolean initialized;
} AppController;

static gboolean controller_open_database(AppController *controller, GError **error) {
  g_autofree gchar *language = NULL;

  app_db_close(&controller->db);

  if (!app_db_open(&controller->db, controller->config.db_path, error)) {
    return FALSE;
  }

  if (!app_db_init_schema(&controller->db, error)) {
    return FALSE;
  }

  language = app_db_get_string(&controller->db, "language");
  if (language) {
    app_config_set_language(&controller->config, language);
  } else {
    if (!app_db_set_string(&controller->db, "language", app_lang_normalize(controller->config.language), error)) {
      return FALSE;
    }
  }

  {
    g_autofree gchar *current_node = app_db_get_string(&controller->db, "current_node");
    if (!current_node) {
      if (!app_db_set_string(&controller->db, "current_node", "intro.overview", error)) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

static void controller_error_close_clicked(GtkButton *button, gpointer user_data) {
  GtkWindow *window = GTK_WINDOW(user_data);
  (void)button;
  gtk_window_destroy(window);
}

static void controller_show_error(GtkApplication *app, const gchar *message) {
  GtkWidget *window = gtk_application_window_new(app);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  GtkWidget *title = gtk_label_new("Error");
  GtkWidget *body = gtk_label_new(message ? message : "Unknown error.");
  GtkWidget *button = gtk_button_new_with_label("Close");

  gtk_window_set_title(GTK_WINDOW(window), "Error");
  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
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
  g_signal_connect(button, "clicked", G_CALLBACK(controller_error_close_clicked), window);
  gtk_window_present(GTK_WINDOW(window));
}

static void controller_present_main_window(AppController *controller) {
  if (!controller->main_window) {
    controller->main_window = main_window_new(controller->app, &controller->config, &controller->db);
  }

  main_window_present(controller->main_window);
}

static void controller_present_wizard(AppController *controller);

static void controller_on_wizard_complete(gpointer user_data) {
  AppController *controller = user_data;
  g_autoptr(GError) error = NULL;

  if (!controller_open_database(controller, &error)) {
    controller_show_error(controller->app, error ? error->message : "Failed to open database.");
    controller_present_wizard(controller);
    return;
  }

  if (controller->wizard_window) {
    wizard_window_destroy(controller->wizard_window);
    controller->wizard_window = NULL;
  }

  controller_present_main_window(controller);
}

static void controller_present_wizard(AppController *controller) {
  if (!controller->wizard_window) {
    controller->wizard_window = wizard_window_new(controller->app,
                                                  &controller->config,
                                                  controller_on_wizard_complete,
                                                  controller);
  }

  wizard_window_present(controller->wizard_window);
}

static void controller_on_activate(GtkApplication *app, gpointer user_data) {
  AppController *controller = user_data;
  g_autoptr(GError) error = NULL;

  controller->app = app;
  app_style_load_for_display(gdk_display_get_default());

  if (controller->initialized) {
    if (controller->main_window) {
      controller_present_main_window(controller);
    } else {
      controller_present_wizard(controller);
    }
    return;
  }

  controller->initialized = TRUE;
  app_config_init_defaults(&controller->config);
  app_config_load(&controller->config);

  if (app_config_has_valid_database(&controller->config) && controller_open_database(controller, &error)) {
    controller_present_main_window(controller);
    return;
  }

  if (error) {
    g_clear_error(&error);
  }

  controller_present_wizard(controller);
}

static void controller_on_shutdown(GApplication *app, gpointer user_data) {
  AppController *controller = user_data;
  (void)app;

  if (controller->main_window) {
    main_window_destroy(controller->main_window);
    controller->main_window = NULL;
  }

  if (controller->wizard_window) {
    wizard_window_destroy(controller->wizard_window);
    controller->wizard_window = NULL;
  }

  app_db_close(&controller->db);
  app_config_clear(&controller->config);
}

int main(int argc, char **argv) {
  AppController controller = {0};
  g_autoptr(GtkApplication) app = NULL;

  g_set_application_name(ML_APP_NAME);
  g_set_prgname(ML_APP_BIN);

  app = gtk_application_new(ML_APP_ID, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(controller_on_activate), &controller);
  g_signal_connect(app, "shutdown", G_CALLBACK(controller_on_shutdown), &controller);

  return g_application_run(G_APPLICATION(app), argc, argv);
}
