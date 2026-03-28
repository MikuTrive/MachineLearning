#ifndef ML_QUIZ_DATA_H
#define ML_QUIZ_DATA_H

#include <glib.h>

typedef struct {
  const gchar *prompt_en;
  const gchar *prompt_zh;
  const gchar *choices_en[3];
  const gchar *choices_zh[3];
  gint correct_index;
  const gchar *explain_en;
  const gchar *explain_zh;
} QuizQuestion;

const QuizQuestion *app_quiz_questions(gsize *count_out);

#endif
