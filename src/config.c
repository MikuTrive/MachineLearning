#include "config.h"

#include "paths.h"

static void set_or_replace(gchar **target, const gchar *value) {
  g_free(*target);
  *target = g_strdup(value);
}

void app_config_init_defaults(AppConfig *config) {
  config->language = g_strdup("en");
  config->db_dir = NULL;
  config->db_path = NULL;
  config->setup_complete = FALSE;
}

void app_config_clear(AppConfig *config) {
  g_clear_pointer(&config->language, g_free);
  g_clear_pointer(&config->db_dir, g_free);
  g_clear_pointer(&config->db_path, g_free);
  config->setup_complete = FALSE;
}

void app_config_set_language(AppConfig *config, const gchar *language) {
  set_or_replace(&config->language, language ? language : "en");
}

void app_config_set_db_dir(AppConfig *config, const gchar *db_dir) {
  g_free(config->db_dir);
  g_free(config->db_path);
  config->db_dir = db_dir ? g_strdup(db_dir) : NULL;
  config->db_path = db_dir ? app_paths_build_database_file(db_dir) : NULL;
}

gboolean app_config_load(AppConfig *config) {
  g_autoptr(GKeyFile) key_file = g_key_file_new();
  g_autofree gchar *config_file = app_paths_get_user_config_file();
  g_autoptr(GError) error = NULL;

  if (!g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_NONE, &error)) {
    return FALSE;
  }

  if (g_key_file_has_key(key_file, "general", "language", NULL)) {
    g_autofree gchar *language = g_key_file_get_string(key_file, "general", "language", NULL);
    app_config_set_language(config, language);
  }

  if (g_key_file_has_key(key_file, "general", "db_dir", NULL)) {
    g_autofree gchar *db_dir = g_key_file_get_string(key_file, "general", "db_dir", NULL);
    app_config_set_db_dir(config, db_dir);
  }

  if (g_key_file_has_key(key_file, "general", "setup_complete", NULL)) {
    config->setup_complete = g_key_file_get_boolean(key_file, "general", "setup_complete", NULL);
  }

  return TRUE;
}

gboolean app_config_save(const AppConfig *config, GError **error) {
  g_autoptr(GKeyFile) key_file = g_key_file_new();
  g_autofree gchar *config_dir = app_paths_get_user_config_dir();
  g_autofree gchar *config_file = app_paths_get_user_config_file();
  gsize data_length = 0;
  g_autofree gchar *data = NULL;

  if (g_mkdir_with_parents(config_dir, 0700) != 0) {
    g_set_error(error,
                g_quark_from_static_string("ml-config"),
                1,
                "Failed to create config directory: %s",
                config_dir);
    return FALSE;
  }

  g_key_file_set_string(key_file, "general", "language", config->language ? config->language : "en");
  g_key_file_set_string(key_file, "general", "db_dir", config->db_dir ? config->db_dir : "");
  g_key_file_set_boolean(key_file, "general", "setup_complete", config->setup_complete);

  data = g_key_file_to_data(key_file, &data_length, error);
  if (!data) {
    return FALSE;
  }

  return g_file_set_contents(config_file, data, (gssize)data_length, error);
}

gboolean app_config_has_valid_database(const AppConfig *config) {
  if (!config || !config->setup_complete || !config->db_dir || !config->db_path) {
    return FALSE;
  }

  if (!app_paths_dir_exists(config->db_dir)) {
    return FALSE;
  }

  if (!app_paths_file_exists(config->db_path)) {
    return FALSE;
  }

  return TRUE;
}
