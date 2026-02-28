CC     := gcc
CFLAGS := -Wall -Wextra -std=c11 -Isrc -lflecs -lraylib -lm
BUILD  := build
TARGET := $(BUILD)/app

# ── auto-detect all modules from src/*.c ─────────────────────────────────
MODULES    := $(basename $(notdir $(wildcard src/*.c)))
OBJ_FILES  := $(MODULES:%=$(BUILD)/%.o)
IMPL_FLAGS := $(MODULES:%=-DIMPL_%)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -DIMPL_$* -c $< -o $@

$(TARGET): $(OBJ_FILES) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

compile_flags.txt: $(wildcard src/*.c) Makefile
	@printf '%s\n' $(CFLAGS) $(IMPL_FLAGS) > $@
	@echo "updated compile_flags.txt"

.PHONY: all clean run lsp module
all: $(TARGET) compile_flags.txt
run: $(TARGET)
	./$(TARGET)
lsp: compile_flags.txt
clean:
	rm -rf $(BUILD)

# ── make module=<name> ───────────────────────────────────────────────────
module:
ifndef module
	$(error Usage: make module=<name>)
endif
	@if [ -f src/$(module).c ]; then \
		echo "error: src/$(module).c already exists"; exit 1; \
	fi
	@printf '#ifndef %s_C\n#define %s_C\n\n/* ── declarations ─────────────────────────────────── */\n\n\n/* ── definitions ──────────────────────────────────── */\n#ifdef IMPL_%s\n\n\n#endif  /* IMPL_%s */\n#endif  /* %s_C */\n' \
		$(shell echo $(module) | tr '[:lower:]' '[:upper:]') \
		$(shell echo $(module) | tr '[:lower:]' '[:upper:]') \
		$(module) \
		$(module) \
		$(shell echo $(module) | tr '[:lower:]' '[:upper:]') \
		> src/$(module).c
	@printf '%s\n' $(CFLAGS) $(IMPL_FLAGS) -DIMPL_$(module) > compile_flags.txt
	@echo "created  src/$(module).c"
	@echo "updated  compile_flags.txt"
