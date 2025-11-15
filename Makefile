CC = gcc

SRC_DIR = src
INC_DIR = include
OBJ_DIR = build
BIN_DIR = bin

CFLAGS = -Wall -g -I${INC_DIR}

TARGET = ${BIN_DIR}/big
SRCS = ${wildcard ${SRC_DIR}/*.c}
OBJS = ${patsubst ${SRC_DIR}/%.c, ${OBJ_DIR}/%.o, ${SRCS}}

all: ${TARGET}

${TARGET}: ${OBJS}
	@mkdir -p ${BIN_DIR}
	${CC} ${OBJS} -o ${TARGET}
	@echo "Link succeeded -> $@"

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	@mkdir -p ${OBJ_DIR}
	${CC} ${CFLAGS} -c $< -o $@
	@echo "Compile succeeded -> $@"

clean:
	rm -rf ${BIN_DIR}/*
	rm -rf ${OBJ_DIR}/*
	@echo "Clean succeeded"

.Phony: all clean