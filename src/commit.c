#include "commit.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "error_handle.h"
#include "snapshot.h"
#include "utils.h"
typedef struct CommitNode {
    SnapshotBST *snapshot;
    char *log;
    char *datetime;
    struct CommitNode **parent;
} CommitNode;

struct CommitGraph {
    CommitNode *nodes;
};

static const char temp_log_filename[] = ".big/temp_log.txt";
static const char objects_dir[] = ".big/objects";

static CommitNode *CommitNodeCreate(const char *log) {
    CommitNode *new_node = (CommitNode *)malloc(sizeof(CommitNode));
    if (new_node == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    size_t size = 0;
    char buffer[100];
    struct tm *datetime_now;
    time_t time_now = time(NULL);

    datetime_now = localtime(&time_now);
    strftime(buffer, 100, "%Y/%m/%d %H:%M:%S", datetime_now);

    new_node->log = str_dup(log);
    new_node->datetime = str_dup(buffer);
    new_node->parent = NULL;
    new_node->snapshot = read_index_file(&size);

    return new_node;
}

static void CommitNodeFree(CommitNode *node) {
    free(node->log);
    node->log = NULL;
    free(node->datetime);
    node->datetime = NULL;
    SnapshotBSTDestory(&(node->snapshot));
}

static void CommitGraphDestory(CommitGraph **graph) {
    // TODO: Free all Graph. Use with CommitNodeFree
}

static char *log_file_handle() {
    FILE *log_file = fopen(temp_log_filename, "rb");
    if (log_file == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    char c;
    while ((c = fgetc(log_file)) != EOF && c != '\n');

    if ((c = fgetc(log_file)) == EOF) {
        fclose(log_file);
        remove(temp_log_filename);
        ErrorCustomMsg("Commit operation cancelled\n");
    }

    size_t start_pos = ftell(log_file) - 1;
    fseek(log_file, 0, SEEK_END);
    size_t end_pos = ftell(log_file);
    size_t content_length = end_pos - start_pos;
    char *log = (char *)malloc(content_length);
    if (log == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    fseek(log_file, start_pos, SEEK_SET);
    fread(log, 1, content_length, log_file);
    log[content_length - 1] = '\0';

    fclose(log_file);
    return log;
}

static char *log_from_editor() {
    pid_t pid;

    pid = fork();

    if (pid == -1)
        ErrorCustomMsg("Error: can not create child process\n");
    else if (pid == 0) {
        char *argv[] = {"nano", temp_log_filename, NULL};
        FILE *temp_log_file = fopen(temp_log_filename, "wb");
        fputs("// Write down your commit log below this line\n", temp_log_file);
        fclose(temp_log_file);
        execvp("nano", argv);
        ErrorCustomMsg("Error: can not open nano editor for commit log\n");
    } else {
        int status;
        wait(&status);
    }

    if (access(temp_log_filename, F_OK) == -1)
        ErrorCustomMsg("Commit operation cancelled\n");

    char *log = log_file_handle();

    remove(temp_log_filename);
    return log;
}

static void save_object_file(CommitNode *node) {
    if (access(objects_dir, F_OK) == -1) {
        if (mkdir(objects_dir, 0775) == -1)
            ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    if (chdir(objects_dir) == -1)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    size_t log_length = strlen(node->log);
    size_t datetime_length = strlen(node->datetime);
    char *pre_hash_string = (char *)malloc(log_length + datetime_length + 1);
    strcpy(pre_hash_string, node->log);
    strcpy(pre_hash_string + log_length, node->datetime);

    char *object_dir = hash_to_string(hash_function(pre_hash_string));
    free(pre_hash_string);

    if (access(object_dir, F_OK) == 0) {
        // TODO: Detect collision and solve it
    }

    if (mkdir(object_dir, 0775) == -1)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    // TODO: Save all content of CommitNode

    free(object_dir);
    cd_to_project_root(NULL);
}

static void leader_update(CommitNode **graph) {
    // TODO: Update Leader file for new commit
}

void commit(const char *log_message) {
    cd_to_project_root(NULL);

    char *log;
    if (log_message == NULL)
        log = log_from_editor();
    else
        log = str_dup(log_message);

    CommitNode *new_commit = CommitNodeCreate(log);

    save_object_file(new_commit);

    leader_update(new_commit);

    CommitNodeFree(new_commit);
    free(new_commit);
    new_commit = NULL;
}