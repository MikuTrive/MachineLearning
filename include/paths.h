#ifndef ML_PATHS_H
#define ML_PATHS_H

#include <glib.h>

const gchar *app_paths_get_data_dir(void);
gchar *app_paths_get_user_config_dir(void);
gchar *app_paths_get_user_config_file(void);
gchar *app_paths_build_database_file(const gchar *db_dir);
gboolean app_paths_dir_exists(const gchar *path);
gboolean app_paths_file_exists(const gchar *path);

#endif
