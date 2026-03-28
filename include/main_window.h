#ifndef ML_MAIN_WINDOW_H
#define ML_MAIN_WINDOW_H

#include <gtk/gtk.h>

#include "config.h"
#include "db.h"

typedef struct MainWindow MainWindow;

MainWindow *main_window_new(GtkApplication *app, AppConfig *config, AppDatabase *db);
void main_window_present(MainWindow *window);
void main_window_destroy(MainWindow *window);

#endif
