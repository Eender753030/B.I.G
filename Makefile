CC = gcc	# Compiler

SRC_DIR = src
INC_DIR = include
OBJ_DIR = build
BIN_DIR = bin
TEST_DIR = tests
TEST_BIN_DIR = ${BIN_DIR}/tests

CFLAGS = -O2 -Wall -Wextra -Wconversion -Wpedantic -Werror -g -fsanitize=address -I${INC_DIR} # Strict compiler to find more error and warrning also check memory leak
LFLAGS = -fsanitize=address

TARGET = ${BIN_DIR}/big

SRCS := ${shell find ${SRC_DIR} -name '*.c'}  # Find all .c files in src/
OBJS := ${SRCS:${SRC_DIR}/%.c=${OBJ_DIR}/%.o} # .c -> .o

MAIN_FILE_NAME = main.c
OBJS_NO_MAIN := $(filter-out $(OBJ_DIR)/$(MAIN_FILE_NAME:.c=.o), $(OBJS)) # Filter the main.c for testing

TEST_SRCS := ${shell find ${TEST_DIR} -name '*.c'} # Find all .c files in test/
TEST_BINS := ${TEST_SRCS:${TEST_DIR}/%.c=${TEST_BIN_DIR}/%} # .c -> .o

all: ${TARGET} # build executable file

${TARGET}: ${OBJS} # Link target
	@mkdir -p ${BIN_DIR}
	${CC} ${LFLAGS} ${OBJS} -o ${TARGET}
	@echo "Link succeeded -> $@"

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c # Compile sources
	@mkdir -p ${dir $@}
	${CC} ${CFLAGS} -c $< -o $@
	@echo "Compile succeeded -> $@"

test: ${TEST_BINS} # build test file

${TEST_BIN_DIR}/%: ${TEST_DIR}/%.c ${OBJS_NO_MAIN} # Compile and link test sources
	@mkdir -p ${dir $@}
	${CC} ${CFLAGS} $< ${OBJS_NO_MAIN} -o $@ ${LFLAGS}
	@echo "Build Test succeeded -> $@"

run_test: test # Run all test in a command
	@echo "--- Running Tests ---"
	@for test_bin in ${TEST_BINS}; do \
		echo "Running $$test_bin ..."; \
		./$$test_bin || exit 1; \
	done
	@echo "--- All Tests Passed ---"

clean: # Clean all builded and compiled files
	rm -rf ${BIN_DIR}/*
	rm -rf ${OBJ_DIR}/*
	@echo "Clean succeeded"

.Phony: all clean