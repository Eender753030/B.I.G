#ifndef CMD_H
#define CMD_H

// Type of command function
typedef void (*cmd_func)(int argc, char *argv[]);

// Structure for a pair of command name and it's function
typedef struct {
    const char *cmd_name;
    cmd_func cmd;
} command_t;

#endif