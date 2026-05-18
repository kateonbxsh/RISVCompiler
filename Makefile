CC      := gcc
FLEX    := flex
YACC    := yacc

SRC_DIR := src
C_DIR   := $(SRC_DIR)/c
TEST    := test/test.program

BUILD_DIR := build
GEN_DIR   := intermediate

CFLAGS  := -Wall -Wextra -Wno-unused-function -I$(C_DIR) -I$(GEN_DIR)
LDFLAGS :=

LEXER_SRC := $(SRC_DIR)/lexer.l
PARSER_SRC := $(SRC_DIR)/yacc.y
C_SOURCES := $(wildcard $(C_DIR)/*.c) $(wildcard $(C_DIR)/code/*.c)

LEXER_C := $(GEN_DIR)/lexer.yy.c
PARSER_C := $(GEN_DIR)/y.tab.c
PARSER_H := $(GEN_DIR)/y.tab.h

PARSER_BIN := $(BUILD_DIR)/yacc

.PHONY: all clean run test test_yacc test_yacc_nobuild build_yacc genlex genh

all: $(PARSER_BIN)

$(BUILD_DIR) $(GEN_DIR):
	mkdir -p $@

$(LEXER_C): $(LEXER_SRC) $(PARSER_H) | $(GEN_DIR)
	$(FLEX) -o $@ $<

$(PARSER_C) $(PARSER_H): $(PARSER_SRC) | $(GEN_DIR)
	$(YACC) -Wcounterexamples -d -o $(PARSER_C) $<

$(PARSER_BIN): $(PARSER_C) $(LEXER_C) $(C_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(PARSER_C) $(LEXER_C) $(C_SOURCES) $(LDFLAGS)

run: $(PARSER_BIN)
	$(PARSER_BIN)

test: test_yacc

test_yacc: $(PARSER_BIN)
	$(PARSER_BIN) < $(TEST)

test_yacc_nobuild:
	$(PARSER_BIN) < $(TEST)

clean:
	rm -rf $(BUILD_DIR) $(GEN_DIR)

genlex: $(LEXER_C)
genh: $(PARSER_H)
build_yacc: $(PARSER_BIN)
