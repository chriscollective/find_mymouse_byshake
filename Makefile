# Builds a single static Windows exe with mingw-w64 (cross-compiles from
# macOS / Linux). Unit tests run natively on the build host.
#
#   make          -> build/find_mymouse_byshake.exe
#   make test     -> run unit tests on the host
#   make clean

CROSS    ?= x86_64-w64-mingw32-
WINCC    := $(CROSS)gcc
WINDRES  := $(CROSS)windres
HOSTCC   ?= cc

BUILD    := build
EXE      := $(BUILD)/find_mymouse_byshake.exe

CORE_SRC := $(wildcard src/core/*.c)
CORE_HDR := $(wildcard src/core/*.h)
WIN_SRC  := $(wildcard src/win/*.c)
WIN_HDR  := $(wildcard src/win/*.h) src/version.h
TEST_SRC := $(wildcard tests/*.c)

WARN     := -Wall -Wextra -Wshadow -Wno-unused-parameter
CFLAGS   ?= -O2
CFLAGS   += -std=c11 $(WARN)
WINDEFS  := -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00
WINLIBS  := -lshcore -ldwmapi -luser32 -lgdi32 -lshell32 -ladvapi32

.PHONY: all test clean

all: $(EXE)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/tests: $(TEST_SRC) $(CORE_SRC) $(CORE_HDR) tests/test_util.h | $(BUILD)
	$(HOSTCC) $(CFLAGS) -Isrc/core $(TEST_SRC) $(CORE_SRC) -lm -o $@

test: $(BUILD)/tests
	./$(BUILD)/tests

$(BUILD)/make_icon: tools/make_icon.c src/core/arrow.c src/core/arrow.h | $(BUILD)
	$(HOSTCC) $(CFLAGS) -Isrc/core tools/make_icon.c src/core/arrow.c -lm -o $@

$(BUILD)/app.ico: $(BUILD)/make_icon
	./$(BUILD)/make_icon $@

$(BUILD)/app.res.o: src/win/app.rc src/win/app.manifest src/win/resource.h src/version.h $(BUILD)/app.ico
	$(WINDRES) -I$(BUILD) -Isrc -Isrc/win -O coff $< -o $@

$(EXE): $(CORE_SRC) $(CORE_HDR) $(WIN_SRC) $(WIN_HDR) $(BUILD)/app.res.o
	$(WINCC) $(CFLAGS) $(WINDEFS) -municode -mwindows -Isrc -Isrc/core -Isrc/win \
		$(CORE_SRC) $(WIN_SRC) $(BUILD)/app.res.o \
		-static -s $(WINLIBS) -o $@

clean:
	rm -rf $(BUILD)
