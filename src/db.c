#include "db.h"

static gboolean exec_sql(AppDatabase *db, const gchar *sql, GError **error) {
  char *errmsg = NULL;
  const int rc = sqlite3_exec(db->handle, sql, NULL, NULL, &errmsg);

  if (rc != SQLITE_OK) {
    g_set_error(error,
                g_quark_from_static_string("ml-db"),
                rc,
                "SQLite error: %s",
                errmsg ? errmsg : "unknown error");
    sqlite3_free(errmsg);
    return FALSE;
  }

  return TRUE;
}

gboolean app_db_open(AppDatabase *db, const gchar *path, GError **error) {
  if (sqlite3_open(path, &db->handle) != SQLITE_OK) {
    g_set_error(error,
                g_quark_from_static_string("ml-db"),
                1,
                "Failed to open database: %s",
                sqlite3_errmsg(db->handle));
    if (db->handle) {
      sqlite3_close(db->handle);
      db->handle = NULL;
    }
    return FALSE;
  }

  g_free(db->path);
  db->path = g_strdup(path);
  return TRUE;
}

void app_db_close(AppDatabase *db) {
  if (db->handle) {
    sqlite3_close(db->handle);
    db->handle = NULL;
  }

  g_clear_pointer(&db->path, g_free);
}

gboolean app_db_init_schema(AppDatabase *db, GError **error) {
  static const gchar *schema_sql =
      "CREATE TABLE IF NOT EXISTS app_meta ("
      "  key TEXT PRIMARY KEY,"
      "  value TEXT NOT NULL"
      ");"
      "CREATE TABLE IF NOT EXISTS lesson_progress ("
      "  node_id TEXT PRIMARY KEY,"
      "  completed INTEGER NOT NULL DEFAULT 0,"
      "  updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
      ");";

  return exec_sql(db, schema_sql, error);
}

gboolean app_db_set_string(AppDatabase *db, const gchar *key, const gchar *value, GError **error) {
  static const gchar *sql =
      "INSERT INTO app_meta(key, value) VALUES(?1, ?2) "
      "ON CONFLICT(key) DO UPDATE SET value = excluded.value;";
  sqlite3_stmt *stmt = NULL;
  const int rc_prepare = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);

  if (rc_prepare != SQLITE_OK) {
    g_set_error(error,
                g_quark_from_static_string("ml-db"),
                rc_prepare,
                "Failed to prepare metadata statement: %s",
                sqlite3_errmsg(db->handle));
    return FALSE;
  }

  sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, value ? value : "", -1, SQLITE_STATIC);

  const int rc_step = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc_step != SQLITE_DONE) {
    g_set_error(error,
                g_quark_from_static_string("ml-db"),
                rc_step,
                "Failed to write metadata: %s",
                sqlite3_errmsg(db->handle));
    return FALSE;
  }

  return TRUE;
}

gchar *app_db_get_string(AppDatabase *db, const gchar *key) {
  static const gchar *sql = "SELECT value FROM app_meta WHERE key = ?1 LIMIT 1;";
  sqlite3_stmt *stmt = NULL;
  gchar *result = NULL;

  if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
    return NULL;
  }

  sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *value = sqlite3_column_text(stmt, 0);
    result = g_strdup((const gchar *)value);
  }

  sqlite3_finalize(stmt);
  return result;
}

gboolean app_db_set_int(AppDatabase *db, const gchar *key, gint value, GError **error) {
  g_autofree gchar *text = g_strdup_printf("%d", value);
  return app_db_set_string(db, key, text, error);
}

gint app_db_get_int(AppDatabase *db, const gchar *key, gint fallback_value) {
  g_autofree gchar *text = app_db_get_string(db, key);
  if (!text) {
    return fallback_value;
  }
  return (gint)g_ascii_strtoll(text, NULL, 10);
}
