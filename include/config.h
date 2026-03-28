#ifndef ML_CONFIG_H
#define ML_CONFIG_H

#include <glib.h>

typedef struct {
  gchar *language;
  gchar *db_dir;
  gchar *db_path;
  gboolean setup_complete;
} AppConfig;

void app_config_init_defaults(AppConfig *config);
void app_config_clear(AppConfig *config);
gboolean app_config_load(AppConfig *config);
gboolean app_config_save(const AppConfig *config, GError **error);
gboolean app_config_has_valid_database(const AppConfig *config);
void app_config_set_language(AppConfig *config, const gchar *language);
void app_config_set_db_dir(AppConfig *config, const gchar *db_dir);

#endif
