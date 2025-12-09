# OS 감지
UNAME_S := $(shell uname -s)

# Windows용 설정
ifeq ($(OS),Windows_NT)
    CC = gcc
    CFLAGS = -Wall -Wextra -g -DDEBUG -Iinclude -Ibuild
    LDFLAGS = -lfl
    EXE_EXT = .exe
    MKDIR = mkdir
else
    CC = gcc
    CFLAGS = -Wall -Wextra -g -DDEBUG -Iinclude -Ibuild
    LDFLAGS = -lfl
    EXE_EXT =
    MKDIR = mkdir -p
endif

BUILD_DIR = build

OBJS = \
	$(BUILD_DIR)/lexer.o \
	$(BUILD_DIR)/parser.tab.o \
	$(BUILD_DIR)/ast.o \
	$(BUILD_DIR)/codegen_x86.o \
	$(BUILD_DIR)/main.o

all: minic_x86$(EXE_EXT)

$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

minic_x86$(EXE_EXT): $(BUILD_DIR) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/parser.tab.c $(BUILD_DIR)/parser.tab.h: parser/parser.y | $(BUILD_DIR)
	bison -d -o $(BUILD_DIR)/parser.tab.c parser/parser.y

$(BUILD_DIR)/lexer.c: parser/lexer.l $(BUILD_DIR)/parser.tab.h | $(BUILD_DIR)
	flex -o $(BUILD_DIR)/lexer.c parser/lexer.l

$(BUILD_DIR)/lexer.o: $(BUILD_DIR)/lexer.c $(BUILD_DIR)/parser.tab.h include/ast.h
	$(CC) $(CFLAGS) -c $(BUILD_DIR)/lexer.c -o $@

$(BUILD_DIR)/parser.tab.o: $(BUILD_DIR)/parser.tab.c include/ast.h
	$(CC) $(CFLAGS) -c $(BUILD_DIR)/parser.tab.c -o $@

$(BUILD_DIR)/ast.o: src/ast.c include/ast.h
	$(CC) $(CFLAGS) -c src/ast.c -o $@

$(BUILD_DIR)/codegen_x86.o: src/codegen_x86.c include/ast.h include/codegen_x86.h
	$(CC) $(CFLAGS) -c src/codegen_x86.c -o $@

$(BUILD_DIR)/main.o: src/main.c include/ast.h include/codegen_x86.h
	$(CC) $(CFLAGS) -c src/main.c -o $@

# 사용자가 out.s를 만든 뒤(= minic_x86 실행해서 저장한 뒤),
#   make prog
# 를 치면 x86-64 실행 파일 prog를 만든다.
prog: out.s
	$(CC) -no-pie out.s -o prog$(EXE_EXT)

# Windows용 빌드 타겟
windows:
	$(MAKE) CC=gcc CFLAGS="-Wall -Wextra -g -DDEBUG -Iinclude -Ibuild" LDFLAGS="-lfl" EXE_EXT=".exe" MKDIR="mkdir"

# Linux/Mac용 빌드 타겟
linux:
	$(MAKE) CC=gcc CFLAGS="-Wall -Wextra -g -DDEBUG -Iinclude -Ibuild" LDFLAGS="-lfl" EXE_EXT="" MKDIR="mkdir -p"

clean:
ifeq ($(OS),Windows_NT)
	-del /Q /F $(BUILD_DIR)\*.* 2>nul || true
	-rmdir /Q /S $(BUILD_DIR) 2>nul || true
	-del /Q /F minic_x86.exe 2>nul || true
	-del /Q /F prog.exe 2>nul || true
else
	rm -rf $(BUILD_DIR) minic_x86$(EXE_EXT) prog$(EXE_EXT)
endif

.PHONY: all clean windows linux
