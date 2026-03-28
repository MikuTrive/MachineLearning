APP_NAME            := Machine Learning
APP_ID              := org.machine.learning
APP_BIN             := machine-learning
PREFIX              ?= /usr/local
BINDIR              := $(PREFIX)/bin
DATADIR             := $(PREFIX)/share
APP_DATADIR         := $(DATADIR)/machine-learning
DESKTOPDIR          := $(DATADIR)/applications
ICONDIR             := $(DATADIR)/icons/hicolor/scalable/apps
PKG_CONFIG          ?= pkg-config
CC                  ?= gcc
BUILD_DIR           := .build
SRC_DIR             := src
INCLUDE_DIR         := include
PKG_MODULES         := gtk4 gtksourceview-5 sqlite3
STD                 := -std=c17
WARNINGS            := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes
OPTIMIZE            ?= -O2
CPPFLAGS            += -D_POSIX_C_SOURCE=200809L \
                       -I$(INCLUDE_DIR)
CFLAGS              += $(STD) $(WARNINGS) $(OPTIMIZE)
LDLIBS              += $(shell $(PKG_CONFIG) --libs $(PKG_MODULES) 2>/dev/null)
PKG_CFLAGS          := $(shell $(PKG_CONFIG) --cflags $(PKG_MODULES) 2>/dev/null)
SOURCES             := $(filter-out $(SRC_DIR)/syntax.c,$(wildcard $(SRC_DIR)/*.c))
OBJECTS             := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS                := $(OBJECTS:.o=.d)

.DEFAULT_GOAL := info

.PHONY: info build install uninstall clean run check-pkg

info:
	@echo "Project..............: $(APP_NAME)"
	@echo "Application ID.......: $(APP_ID)"
	@echo "Binary name..........: $(APP_BIN)"
	@echo "Compiler.............: $(CC)"
	@echo "Language standard....: C17"
	@echo "Optimization.........: $(OPTIMIZE)"
	@echo "Warning flags........: $(WARNINGS)"
	@echo "Install prefix.......: $(PREFIX)"
	@echo "Binary install dir...: $(BINDIR)"
	@echo "Data install dir.....: $(APP_DATADIR)"
	@echo "Desktop file dir.....: $(DESKTOPDIR)"
	@echo "Icon dir.............: $(ICONDIR)"
	@echo "pkg-config modules...: $(PKG_MODULES)"
	@echo "Build command........: make build"
	@echo "Clean .Builds........: make clean"
	@echo "Run command..........: make run"
	@echo "Install command......: sudo make install (build included)"
	@echo "Uninstall command....: sudo make uninstall"

check-pkg:
	@$(PKG_CONFIG) --exists $(PKG_MODULES) || { \
		echo "Missing required development packages: $(PKG_MODULES)"; \
		echo "Please read README.md for distro-specific install commands."; \
		exit 1; \
	}

build: check-pkg $(BUILD_DIR)/$(APP_BIN)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(APP_BIN): $(OBJECTS)
	@echo "Linking $@"
	@$(CC) $(OBJECTS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<"
	@$(CC) $(CPPFLAGS) $(PKG_CFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: build
	@ML_RESOURCE_ROOT="$(CURDIR)/data" ./$(BUILD_DIR)/$(APP_BIN)

install: build
	@install -d "$(DESTDIR)$(BINDIR)"
	@install -d "$(DESTDIR)$(APP_DATADIR)/css"
	@install -d "$(DESTDIR)$(APP_DATADIR)/titlebar-icons"
	@install -d "$(DESTDIR)$(APP_DATADIR)/ui-icons"
	@install -d "$(DESTDIR)$(DESKTOPDIR)"
	@install -d "$(DESTDIR)$(ICONDIR)"
	@install -m 0755 "$(BUILD_DIR)/$(APP_BIN)" "$(DESTDIR)$(BINDIR)/$(APP_BIN)"
	@install -m 0644 "data/css/app.css" "$(DESTDIR)$(APP_DATADIR)/css/app.css"
	@install -m 0644 "data/titlebar-icons/minimize.svg" "$(DESTDIR)$(APP_DATADIR)/titlebar-icons/minimize.svg"
	@install -m 0644 "data/titlebar-icons/maximize.svg" "$(DESTDIR)$(APP_DATADIR)/titlebar-icons/maximize.svg"
	@install -m 0644 "data/titlebar-icons/close.svg" "$(DESTDIR)$(APP_DATADIR)/titlebar-icons/close.svg"
	@install -m 0644 "data/ui-icons/run.svg" "$(DESTDIR)$(APP_DATADIR)/ui-icons/run.svg"
	@install -m 0644 "data/ui-icons/gear.svg" "$(DESTDIR)$(APP_DATADIR)/ui-icons/gear.svg"
	@install -m 0644 "data/ui-icons/chevron-up.svg" "$(DESTDIR)$(APP_DATADIR)/ui-icons/chevron-up.svg"
	@install -m 0644 "data/ui-icons/chevron-down.svg" "$(DESTDIR)$(APP_DATADIR)/ui-icons/chevron-down.svg"
	@install -m 0644 "data/machine-learning.desktop" "$(DESTDIR)$(DESKTOPDIR)/machine-learning.desktop"
	@install -m 0644 "assets/machine-learning.svg" "$(DESTDIR)$(ICONDIR)/machine-learning.svg"
	@command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$(DESTDIR)$(DESKTOPDIR)" || true
	@command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -q -t -f "$(DESTDIR)$(DATADIR)/icons/hicolor" || true
	@echo "Installed $(APP_NAME) to $(DESTDIR)$(PREFIX)"

uninstall:
	@rm -f "$(DESTDIR)$(BINDIR)/$(APP_BIN)"
	@rm -f "$(DESTDIR)$(DESKTOPDIR)/machine-learning.desktop"
	@rm -f "$(DESTDIR)$(ICONDIR)/machine-learning.svg"
	@rm -f "$(DESTDIR)$(APP_DATADIR)/css/app.css"
	@rm -f "$(DESTDIR)$(APP_DATADIR)/titlebar-icons/minimize.svg"
	@rm -f "$(DESTDIR)$(APP_DATADIR)/titlebar-icons/maximize.svg"
	@rm -f "$(DESTDIR)$(APP_DATADIR)/titlebar-icons/close.svg"
	@rm -f "$(DESTDIR)$(APP_DATADIR)/ui-icons/run.svg"
	@rm -f "$(DESTDIR)$(APP_DATADIR)/ui-icons/gear.svg"
	@rm -f "$(DESTDIR)$(APP_DATADIR)/ui-icons/chevron-up.svg"
	@rm -f "$(DESTDIR)$(APP_DATADIR)/ui-icons/chevron-down.svg"
	@rmdir --ignore-fail-on-non-empty "$(DESTDIR)$(APP_DATADIR)/css" 2>/dev/null || true
	@rmdir --ignore-fail-on-non-empty "$(DESTDIR)$(APP_DATADIR)/titlebar-icons" 2>/dev/null || true
	@rmdir --ignore-fail-on-non-empty "$(DESTDIR)$(APP_DATADIR)/ui-icons" 2>/dev/null || true
	@rmdir --ignore-fail-on-non-empty "$(DESTDIR)$(APP_DATADIR)" 2>/dev/null || true
	@command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$(DESTDIR)$(DESKTOPDIR)" || true
	@command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -q -t -f "$(DESTDIR)$(DATADIR)/icons/hicolor" || true
	@echo "Uninstalled $(APP_NAME) from $(DESTDIR)$(PREFIX)"

clean:
	@rm -rf $(BUILD_DIR)

-include $(DEPS)
