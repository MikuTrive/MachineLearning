#ifndef ML_SYNTAX_H
#define ML_SYNTAX_H

#include <gtksourceview/gtksource.h>

GtkSourceLanguageManager *app_syntax_create_language_manager(void);
GtkSourceStyleSchemeManager *app_syntax_create_style_scheme_manager(void);

#endif
