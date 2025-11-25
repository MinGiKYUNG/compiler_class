CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude -Ibuild
BISON   = bison
FLEX    = flex

BUILD_DIR  = build
SRC_DIR    = src
PARSER_DIR = parser

OBJS = $(BUILD_DIR)/parser.tab.o \
       $(BUILD_DIR)/lex.yy.o \
       $(SRC_DIR)/ast.o \
       $(SRC_DIR)/codegen_x86.o \
       $(SRC_DIR)/main.o

TARGET = minic_x86

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/parser.tab.c $(BUILD_DIR)/parser.tab.h: $(PARSER_DIR)/parser.y | $(BUILD_DIR)
	$(BISON) -d -o $(BUILD_DIR)/parser.tab.c $(PARSER_DIR)/parser.y

$(BUILD_DIR)/lex.yy.c: $(PARSER_DIR)/scanner.l $(BUILD_DIR)/parser.tab.h | $(BUILD_DIR)
	$(FLEX) -o $(BUILD_DIR)/lex.yy.c $(PARSER_DIR)/scanner.l

$(BUILD_DIR)/parser.tab.o: $(BUILD_DIR)/parser.tab.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lex.yy.o: $(BUILD_DIR)/lex.yy.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# 사용자가 out.s를 만든 뒤(= minic_x86 실행해서 저장한 뒤),
#   make prog
# 를 치면 x86-64 실행 파일 prog를 만든다.
prog: out.s
	$(CC) -no-pie out.s -o prog

clean:
	rm -rf $(BUILD_DIR) $(SRC_DIR)/*.o $(TARGET) out.s prog

.PHONY: all clean prog
