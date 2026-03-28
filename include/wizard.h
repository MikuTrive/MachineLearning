#ifndef ML_WIZARD_H
#define ML_WIZARD_H

#include <gtk/gtk.h>

#include "config.h"

typedef struct WizardWindow WizardWindow;
typedef void (*WizardCompleteCallback)(gpointer user_data);

WizardWindow *wizard_window_new(GtkApplication *app,
                                AppConfig *config,
                                WizardCompleteCallback callback,
                                gpointer user_data);
void wizard_window_present(WizardWindow *wizard);
void wizard_window_destroy(WizardWindow *wizard);

#endif
