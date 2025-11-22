#ifndef CMD_H
#define CMD_H

#include "commands/cmd_add.h"
#include "commands/cmd_commit.h"
#include "commands/cmd_init.h"
#include "commands/cmd_log.h"
#include "commands/cmd_status.h"

typedef void (*cmd_func)(int argc, char *argv[]);

typedef struct {
    const char *cmd_name;
    cmd_func cmd;
} Command;

const Command commands[] = {{"init", cmd_init}, {"add", cmd_add},       {"commit", cmd_commit},
                            {"log", cmd_log},   {"status", cmd_status}, {NULL, NULL}};

#endif