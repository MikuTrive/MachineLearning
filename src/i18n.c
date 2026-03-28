#include "i18n.h"

#include <string.h>

typedef struct {
  const gchar *key;
  const gchar *en;
  const gchar *zh_cn;
} TranslationEntry;

static const TranslationEntry TRANSLATIONS[] = {
    {"app.name", "Machine Learning", "Machine Learning"},
    {"wizard.window_title", "Machine Learning · First Launch", "Machine Learning · 首次启动"},
    {"wizard.title", "Welcome to Machine Learning", "欢迎使用 Machine Learning"},
    {"wizard.subtitle", "A native Linux desktop teacher for the Machine programming language.", "一个面向 Linux 的原生 Machine 编程语言教学桌面软件。"},
    {"wizard.language.title", "Choose your interface language", "选择界面语言"},
    {"wizard.language.body", "English is selected by default. You can switch instantly before continuing.", "默认语言为英语。继续之前你可以立即切换显示语言。"},
    {"wizard.language.english", "English", "英语"},
    {"wizard.language.zh_cn", "Simplified Chinese", "简体中文"},
    {"wizard.next", "Next", "下一步"},
    {"wizard.back", "Back", "上一步"},
    {"wizard.database.title", "Where should you store your database?", "将你的数据库存放到何处？"},
    {"wizard.database.body", "Your progress, lesson state, and resume point will be stored there. If the database directory is deleted later, onboarding starts again.", "你的进度、教学节点状态和断点恢复信息都会存放在那里。如果之后数据库目录被删除，程序会重新进入首次引导。"},
    {"wizard.database.note", "The selected directory will hold the local SQLite database used by this teaching software.", "所选目录将保存本教学软件使用的本地 SQLite 数据库。"},
    {"wizard.database.browse", "Choose Folder", "选择目录"},
    {"wizard.database.confirm", "Finish Setup", "完成设置"},
    {"wizard.database.empty", "Please choose a directory before continuing.", "继续之前请先选择一个目录。"},
    {"wizard.database.selected", "Selected database directory", "已选择的数据库目录"},
    {"wizard.database.none", "No folder selected yet.", "还没有选择目录。"},
    {"wizard.error", "Error", "错误"},
    {"wizard.setup_failed", "Setup could not be completed.", "初始化设置未能完成。"},
    {"wizard.choose_folder_title", "Choose the database folder", "选择数据库目录"},

    {"list1.window_title", "Machine Learning · List1", "Machine Learning · List1"},
    {"list1.page_title", "List1", "List1"},
    {"list1.page_subtitle", "Expand the cards you need and solve each lesson with your own Machine code.", "展开你需要的模块，然后用你自己的 Machine 代码通过每一关。"},
    {"list1.lessons", "Learning Nodes", "学习节点"},
    {"list1.current_node", "Current node", "当前节点"},
    {"list1.database_path", "Database path", "数据库路径"},
    {"list1.language", "Interface language", "界面语言"},
    {"list1.compiler_root", "Machine runtime root", "Machine 运行时根目录"},
    {"list1.editor_title", "Machine code editor", "Machine 代码编辑器"},
    {"list1.editor_subtitle", "This lesson editor saves while you type and mirrors a runnable .mne file beside your selected database.", "这个课程编辑器会在你输入时自动保存，并把可运行的 .mne 文件镜像到你所选数据库旁边。"},
    {"list1.run", "Run", "运行"},
    {"list1.terminal_title", "Terminal Output", "终端输出"},
    {"list1.terminal_empty", "Compile and run the lesson to see compiler output and program output here.", "编译并运行当前关卡后，编译器输出和程序输出会显示在这里。"},
    {"list1.status_resume", "The app restores your last node automatically on the next launch.", "程序会在下次启动时自动恢复到你上次停留的节点。"},
    {"list1.status_autosave", "Lesson code is saved automatically to the database and mirrored into real files in the database folder.", "课程代码会自动保存到数据库，并同步为数据库目录里的真实文件。"},
    {"module.lesson_notes", "Lesson Notes", "课程讲解"},
    {"module.current_node", "Current Node", "当前节点"},
    {"module.headers", "Precompiled Headers", "预编译头"},
    {"module.example_code", "Example Snippet", "示例代码"},
    {"module.goal", "Challenge Goal", "关卡目标"},
    {"module.toggle_expand", "Expand", "展开"},
    {"module.toggle_collapse", "Collapse", "收起"},
    {"module.code_hint", "This example is intentionally incomplete. You still need to finish the lesson yourself.", "这个示例故意没有写完整，真正通过关卡还需要你自己补完。"},
    {"module.example_empty", "This page does not need an example snippet.", "这个页面不需要额外示例代码。"},
    {"badge.normal", "Normal", "普通"},
    {"badge.hard", "Hard", "困难"},
    {"badge.extreme", "Extreme", "超难"},
    {"badge.advanced", "Advanced", "进阶"},

    {"titlebar.settings", "Settings", "设置"},
    {"titlebar.minimize", "Minimize", "最小化"},
    {"titlebar.maximize", "Maximize", "最大化"},
    {"titlebar.close", "Close", "关闭"},
    {"settings.window_title", "Settings Panel", "设置面板"},
    {"settings.system_language", "System Language", "系统语言"},
    {"settings.select_language", "Select Language", "选择语言"},
    {"settings.save", "Save", "保存"},
    {"settings.saved", "Saved and applied immediately.", "已保存并立即生效。"},
    {"settings.current_selection", "Current selection", "当前选择"},
    {"settings.close_panel", "Close Settings Panel", "关闭设置面板"},

    {"banner.passed", "Passed", "已通过"},
    {"banner.compile_error", "Compile error", "编译错误"},
    {"banner.runtime_error", "Runtime error", "运行错误"},
    {"banner.output_mismatch", "Correct, but the output or required syntax does not match the goal yet. Please try again.", "正确，但输出结果或要求语法还没有完全达到目标。请再试一次"},

    {"run.machine_missing", "The Machine compiler could not be found in PATH or at /usr/local/bin/machine.", "未在 PATH 或 /usr/local/bin/machine 中找到 Machine 编译器。"},
    {"run.tempdir_failed", "Failed to prepare the lesson working directory beside your database.", "无法在数据库旁边准备本关卡的工作目录。"},
    {"run.write_failed", "Failed to write your lesson code into the lesson .mne file beside your database.", "无法把你的课程代码写入数据库旁边的课程 .mne 文件。"},
    {"run.compile_failed", "Failed to start the Machine compiler process.", "启动 Machine 编译器进程失败。"},
    {"run.execute_failed", "The compiled lesson program could not be executed.", "无法执行已经编译出的课程程序。"},
    {"run.compile_exit", "Compiler output (non-zero exit status):", "编译器输出（非零退出状态）："},
    {"run.missing_items", "The program ran, but this lesson still expects these syntax elements:", "程序已经运行，但本关仍然要求你显式写出这些语法元素："},
    {"run.saved_files", "Saved lesson files", "已保存的课程文件"},
    {"banner.compile_speed_pass", "Passed. This compile took %d ms.", "已通过。本次编译速率 %d ms。"},
    {"banner.compile_speed_fail", "Correct, but the compile speed target was not reached yet. This compile took %d ms.", "正确，但编译速率还没有达标。本次编译速率 %d ms。"},
    {"banner.run_speed_pass", "Passed. This run took %d ms.", "已通过。本次运行速率 %d ms。"},
    {"banner.run_speed_fail", "Correct, but the run speed target was not reached yet. This run took %d ms.", "正确，但运行速率还没有达标。本次运行速率 %d ms。"},
    {"banner.program_speed_pass", "Passed. Compile %d ms / run %d ms.", "已通过。本次编译速率 %d ms，运行速率 %d ms。"},
    {"banner.program_speed_fail", "Correct, but the program performance target was not reached yet. Compile %d ms / run %d ms.", "正确，但程序性能还没有达标。本次编译速率 %d ms，运行速率 %d ms。"},
    {"run.compile_time", "Compile time", "编译耗时"},
    {"run.run_time", "Run time", "运行耗时"},
    {"run.program_time", "Program timing", "程序计时"},
    {"quiz.title", "Multiple Choice", "选择题"},
    {"quiz.subtitle", "Answer 50 Machine 2.0 questions. Each question is worth 2 points, for a total of 100.", "完成 50 道 Machine 2.0 选择题。每题 2 分，总分 100 分。"},
    {"quiz.submit", "Submit", "提交"},
    {"quiz.answer_all", "Answer all 50 questions before submitting.", "请先完成全部 50 道题，再提交。"},
    {"quiz.score", "Score", "得分"},
    {"quiz.total", "Total", "总分"},
    {"quiz.ready", "You can submit after every question has an answer.", "当所有题目都已经选择答案后，你就可以提交。"},
    {"quiz.all_correct", "Excellent. You got all 50 questions correct.", "很好，你 50 题全对。"},
    {"quiz.feedback_header", "Wrong answers and explanations", "错题与解析"},
    {"quiz.result_format", "Score: %d / 100", "得分：%d / 100"},
    {"quiz.q_format", "Question (%d)", "第（%d）题"},
    {"quiz.correct_answer", "Correct answer", "正确答案"},
    {"quiz.your_answer", "Your answer", "你的答案"},
    {"quiz.unanswered", "Unanswered", "未作答"},
    {NULL, NULL, NULL}};

const gchar *app_lang_normalize(const gchar *language) {
  if (!language) {
    return "en";
  }
  if (g_ascii_strcasecmp(language, "zh-CN") == 0 ||
      g_ascii_strcasecmp(language, "zh_CN") == 0 ||
      g_ascii_strcasecmp(language, "zh") == 0) {
    return "zh-CN";
  }
  return "en";
}

const gchar *app_i18n_get(const gchar *language, const gchar *key) {
  const gchar *normalized = app_lang_normalize(language);
  for (gsize index = 0; TRANSLATIONS[index].key != NULL; ++index) {
    if (strcmp(TRANSLATIONS[index].key, key) == 0) {
      return strcmp(normalized, "zh-CN") == 0 ? TRANSLATIONS[index].zh_cn : TRANSLATIONS[index].en;
    }
  }
  return key;
}
