#include "syntax.h"

#include "paths.h"

GtkSourceLanguageManager *app_syntax_create_language_manager(void) {
  GtkSourceLanguageManager *manager = gtk_source_language_manager_new();
  gchar *language_dir = g_build_filename(app_paths_get_data_dir(), "language-specs", NULL);
  gchar *search_paths[] = {language_dir, NULL};

  gtk_source_language_manager_set_search_path(manager, (const gchar * const *)search_paths);
  g_free(language_dir);
  return manager;
}

GtkSourceStyleSchemeManager *app_syntax_create_style_scheme_manager(void) {
  GtkSourceStyleSchemeManager *manager = gtk_source_style_scheme_manager_new();
  gchar *style_dir = g_build_filename(app_paths_get_data_dir(), "styles", NULL);
  gchar *search_paths[] = {style_dir, NULL};

  gtk_source_style_scheme_manager_set_search_path(manager, (const gchar * const *)search_paths);
  g_free(style_dir);
  return manager;
}
