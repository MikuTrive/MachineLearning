#include "style.h"

#include <gtk/gtk.h>

#include "paths.h"

void app_style_load_for_display(GdkDisplay *display) {
  g_autoptr(GtkCssProvider) provider = gtk_css_provider_new();
  g_autofree gchar *css_path = g_build_filename(app_paths_get_data_dir(), "css", "app.css", NULL);

  if (!display) {
    return;
  }

  gtk_css_provider_load_from_path(provider, css_path);
  gtk_style_context_add_provider_for_display(display,
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
