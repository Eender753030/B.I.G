CC = gcc

SRC_DIR = src
INC_DIR = include
OBJ_DIR = build
BIN_DIR = bin

CFLAGS = -Wall -Wextra -Wconversion -Wpedantic -Werror -g -fsanitize=address -I${INC_DIR}
LFLAGS = -fsanitize=address

TARGET = ${BIN_DIR}/big

SRCS := ${shell find ${SRC_DIR} -name '*.c'}
OBJS := ${SRCS:${SRC_DIR}/%.c=${OBJ_DIR}/%.o}

all: ${TARGET}

${TARGET}: ${OBJS}
	@mkdir -p ${BIN_DIR}
	${CC} ${LFLAGS} ${OBJS} -o ${TARGET}
	@echo "Link succeeded -> $@"

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGS} -c $< -o $@
	@echo "Compile succeeded -> $@"

clean:
	rm -rf ${BIN_DIR}/*
	rm -rf ${OBJ_DIR}/*
	@echo "Clean succeeded"

.Phony: all clean