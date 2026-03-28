#include "paths.h"
#include "app_meta.h"

#include <glib/gstdio.h>

const gchar *app_paths_get_data_dir(void) {
  const gchar *override = g_getenv("ML_RESOURCE_ROOT");
  return (override && *override) ? override : ML_APP_DATA_DIR;
}

gchar *app_paths_get_user_config_dir(void) {
  return g_build_filename(g_get_user_config_dir(), "machine-learning", NULL);
}

gchar *app_paths_get_user_config_file(void) {
  g_autofree gchar *config_dir = app_paths_get_user_config_dir();
  return g_build_filename(config_dir, "config.ini", NULL);
}

gchar *app_paths_build_database_file(const gchar *db_dir) {
  return g_build_filename(db_dir, "machine_learning.db", NULL);
}

gboolean app_paths_dir_exists(const gchar *path) {
  return g_file_test(path, G_FILE_TEST_IS_DIR);
}

gboolean app_paths_file_exists(const gchar *path) {
  return g_file_test(path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);
}
