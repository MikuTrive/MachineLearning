#ifndef ML_DB_H
#define ML_DB_H

#include <glib.h>
#include <sqlite3.h>

typedef struct {
  sqlite3 *handle;
  gchar *path;
} AppDatabase;

gboolean app_db_open(AppDatabase *db, const gchar *path, GError **error);
void app_db_close(AppDatabase *db);
gboolean app_db_init_schema(AppDatabase *db, GError **error);
gboolean app_db_set_string(AppDatabase *db, const gchar *key, const gchar *value, GError **error);
gchar *app_db_get_string(AppDatabase *db, const gchar *key);
gboolean app_db_set_int(AppDatabase *db, const gchar *key, gint value, GError **error);
gint app_db_get_int(AppDatabase *db, const gchar *key, gint fallback_value);

#endif
