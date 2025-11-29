CC = gcc

SRC_DIR = src
INC_DIR = include
OBJ_DIR = build
BIN_DIR = bin
TEST_DIR = tests
TEST_BIN_DIR = ${BIN_DIR}/tests

CFLAGS = -Wall -Wextra -Wconversion -Wpedantic -Werror -g -fsanitize=address -I${INC_DIR}
LFLAGS = -fsanitize=address

TARGET = ${BIN_DIR}/big

SRCS := ${shell find ${SRC_DIR} -name '*.c'}
OBJS := ${SRCS:${SRC_DIR}/%.c=${OBJ_DIR}/%.o}

MAIN_FILE_NAME = main.c
OBJS_NO_MAIN := $(filter-out $(OBJ_DIR)/$(MAIN_FILE_NAME:.c=.o), $(OBJS))

TEST_SRCS := ${shell find ${TEST_DIR} -name '*.c'}
TEST_BINS := ${TEST_SRCS:${TEST_DIR}/%.c=${TEST_BIN_DIR}/%}

all: ${TARGET}

${TARGET}: ${OBJS}
	@mkdir -p ${BIN_DIR}
	${CC} ${LFLAGS} ${OBJS} -o ${TARGET}
	@echo "Link succeeded -> $@"

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGS} -c $< -o $@
	@echo "Compile succeeded -> $@"

test: ${TEST_BINS}

${TEST_BIN_DIR}/%: ${TEST_DIR}/%.c ${OBJS_NO_MAIN}
	@mkdir -p ${dir $@}
	${CC} ${CFLAGS} $< ${OBJS_NO_MAIN} -o $@ ${LFLAGS}
	@echo "Build Test succeeded -> $@"

run_test: test
	@echo "--- Running Tests ---"
	@for test_bin in ${TEST_BINS}; do \
		echo "Running $$test_bin ..."; \
		./$$test_bin || exit 1; \
	done
	@echo "--- All Tests Passed ---"

clean:
	rm -rf ${BIN_DIR}/*
	rm -rf ${OBJ_DIR}/*
	@echo "Clean succeeded"

.Phony: all clean