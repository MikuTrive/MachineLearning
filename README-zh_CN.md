# Machine Learning

[简体中文](./README-zh_CN.md) | [English](./README.md)

**Machine Learning** 是一个面向 **Machine** 编程语言的原生 Linux 桌面教学应用，使用 **C**、**GTK4**、**GtkSourceView 5** 和 **SQLite3** 编写。

当前这个第一阶段脚手架已经实现了以下内容：

- 仅面向 Linux 的桌面打包
- 首次启动引导向导
- 英文 / 简体中文界面即时切换
- 由用户自行选择数据库目录
- 基于 SQLite 的“从上次学习节点继续”持久化
- 一个持久存在的主窗口 **List1**
- 支持普通 `.mne` 文本编辑、行号显示，以及与课程节点关联的运行 / 检查工作流
- 安装 / 卸载目标在安装完成后**不依赖源代码目录**
- 与位于 `/usr/local/lib/machine` 和 `/usr/local/include/machine` 下的 Machine 编译器运行时路径相隔离

## 设计说明

- 引导向导窗口为固定大小，不可调整。
- 主窗口是长期存在的工作区窗口，名称为 **List1**。
- 如果用户后来删除了所选数据库目录，那么应用在下次启动时会重新进入首次引导向导。
- 应用会将自己的资源安装到 `/usr/local/share/machine-learning`，**不会**覆盖或迁移 Machine 编译器运行时。
- 代码编辑器保持普通纯文本编辑行为，不会替换或修改 Machine 编译器本体安装内容。

## 窗口尺寸映射

GTK 窗口使用像素尺寸，而原始产品说明中给出的窗口大小是 `100x35` 与 `145x45`。

当前脚手架将它们映射为更适合桌面环境的默认像素尺寸：

- 引导向导：**1000 x 700**
- List1 主窗口：**1450 x 900**

如果你后续想调整成别的映射方式，也很容易修改。

## 依赖项

### Fedora

```bash
sudo dnf install -y gcc make gtk4-devel gtksourceview5-devel sqlite-devel desktop-file-utils
```

### Debian / Ubuntu

```bash
sudo apt install -y build-essential libgtk-4-dev libgtksourceview-5-dev libsqlite3-dev desktop-file-utils
```

### Arch Linux

```bash
sudo pacman -S --needed base-devel gtk4 gtksourceview5 sqlite desktop-file-utils
```

## 构建命令

直接运行 `make` 会打印英文版的构建参数摘要：

```bash
make
```

构建应用：

```bash
make build
```

从源码目录直接运行：

```bash
make run
```

系统范围安装：

```bash
sudo make install
```

卸载：

```bash
sudo make uninstall
```

## 安装后的目录布局

安装完成后，项目将不再依赖源码目录。

```text
/usr/local/bin/machine-learning
/usr/local/share/machine-learning/css/app.css
/usr/local/share/machine-learning/language-specs/machine.lang
/usr/local/share/applications/machine-learning.desktop
/usr/local/share/icons/hicolor/scalable/apps/machine-learning.svg
```

## 配置文件与数据库

用户配置文件存放在：

```text
~/.config/machine-learning/config.ini
```

实际的课程数据库文件会存放在用户在首次引导时选择的目录中：

```text
<用户选择的目录>/machine_learning.db
```

## 当前已可用功能

- 首次启动语言选择
- 界面语言即时切换
- 数据库目录选择
- SQLite 初始化
- 当前课程节点持久化
- 自动恢复到上一次学习节点
- 后续启动时可直接进入 List1 主工作区

## 下一步计划

这个脚手架已经为下一个阶段做好准备：

- 完整课程图谱
- 更丰富的课程内容渲染
- Machine 代码执行集成
- 学习进度检查点与通过 / 未通过跟踪
- 更高级的编辑器能力
- 更深入的类 HTML / CSS 卡片布局与学习模块
