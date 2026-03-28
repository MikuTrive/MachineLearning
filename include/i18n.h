#ifndef ML_I18N_H
#define ML_I18N_H

#include <glib.h>

const gchar *app_i18n_get(const gchar *language, const gchar *key);
const gchar *app_lang_normalize(const gchar *language);

#endif
