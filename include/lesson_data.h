#ifndef ML_LESSON_DATA_H
#define ML_LESSON_DATA_H

#include <glib.h>

typedef enum {
  LESSON_DIFFICULTY_NONE = 0,
  LESSON_DIFFICULTY_NORMAL,
  LESSON_DIFFICULTY_HARD,
  LESSON_DIFFICULTY_EXTREME,
  LESSON_DIFFICULTY_ADVANCED
} LessonDifficulty;

typedef enum {
  LESSON_MODE_INFO = 0,
  LESSON_MODE_OUTPUT,
  LESSON_MODE_BENCH_COMPILE,
  LESSON_MODE_BENCH_RUN,
  LESSON_MODE_BENCH_PROGRAM,
  LESSON_MODE_QUIZ
} LessonMode;

typedef struct {
  const gchar *id;
  const gchar *slug;
  gboolean challenge;
  LessonDifficulty difficulty;
  LessonMode mode;
  gint compile_target_ms;
  gint run_target_ms;
  const gchar *title_en;
  const gchar *title_zh;
  const gchar *summary_en;
  const gchar *summary_zh;
  const gchar *notes_en;
  const gchar *notes_zh;
  const gchar *headers_en;
  const gchar *headers_zh;
  const gchar *goal_en;
  const gchar *goal_zh;
  const gchar *example_code;
  const gchar *editor_default;
  const gchar *expected_output;
  const gchar *required_tokens[16];
} LessonSpec;

const LessonSpec *app_lessons_all(gsize *count_out);
const LessonSpec *app_lesson_find(const gchar *id);
const gchar *app_lesson_title(const LessonSpec *lesson, const gchar *language);
const gchar *app_lesson_summary(const LessonSpec *lesson, const gchar *language);
const gchar *app_lesson_notes(const LessonSpec *lesson, const gchar *language);
const gchar *app_lesson_headers(const LessonSpec *lesson, const gchar *language);
const gchar *app_lesson_goal(const LessonSpec *lesson, const gchar *language);
const gchar *app_lesson_difficulty_text(LessonDifficulty difficulty, const gchar *language);
const gchar *app_lesson_difficulty_css_class(LessonDifficulty difficulty);

#endif
