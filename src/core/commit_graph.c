#include "core/commit_graph.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "core/snapshot.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

static const char temp_log_filename[] = ".big/temp_log.txt";
static const char objects_dir[] = ".big/objects";

char *load_leader() {
    FILE *leader = xfopen(".big/Leader", "r");

    fseek(leader, 0, SEEK_END);
    size_t leader_id_length = (size_t)ftell(leader);
    if (leader_id_length == 0) {
        fclose(leader);
        return NULL;
    }
    fseek(leader, 0, SEEK_SET);

    char *leader_id = xmalloc(leader_id_length + 1);

    fgets(leader_id, (int)leader_id_length, leader);
    leader_id[leader_id_length] = '\0';

    fclose(leader);
    return leader_id;
}

CommitNode *load_parent_info(char *commit_id) {
    CommitNode *parent_node = xmalloc(sizeof(*parent_node));

    parent_node->commit_id = commit_id;

    char parent_dir[1024];
    snprintf(parent_dir, 1024, "%s/%s/%s", objects_dir, commit_id, "info");

    FILE *parent_info = xfopen(parent_dir, "r");

    char buffer[128] = {0};

    fgets(buffer, 128, parent_info);
    buffer[strcspn(buffer, "\n")] = '\0';
    parent_node->datetime = str_dup(buffer);

    if (fgets(buffer, 128, parent_info) != NULL) {
        if (strchr(buffer, ',')) {
            sscanf(buffer, "%lu, %s\n", &(parent_node->parent_num), buffer);
            parent_node->parent = xmalloc(sizeof(*parent_node->parent));
            char *parent_commit_id = str_dup(buffer);
            parent_node->parent[0] = load_parent_info(parent_commit_id);
        } else {
            parent_node->parent_num = 0;
            parent_node->parent = NULL;
        }
    }

    size_t current_pos = (size_t)ftell(parent_info);
    fseek(parent_info, 0, SEEK_END);
    size_t log_length = (size_t)ftell(parent_info) - current_pos - 1;
    parent_node->log = xmalloc(log_length + 1);
    fseek(parent_info, (long)current_pos, SEEK_SET);
    fread(parent_node->log, 1, log_length, parent_info);
    parent_node->log[log_length] = '\0';

    parent_node->snapshot = NULL;

    fclose(parent_info);

    return parent_node;
}

CommitNode *CommitNodeCreate(char *log) {
    CommitNode *new_node = xmalloc(sizeof(*new_node));

    size_t size = 0;
    char buffer[100];
    struct tm *datetime_now;
    time_t time_now = time(NULL);

    datetime_now = localtime(&time_now);
    strftime(buffer, 100, "%Y/%m/%d %H:%M:%S", datetime_now);

    new_node->log = log;
    new_node->datetime = str_dup(buffer);
    new_node->snapshot = read_index_file(&size);

    if (access(".big/Leader", F_OK) == 0) {
        new_node->parent = xmalloc(sizeof(*new_node->parent));
        new_node->parent_num = 1;
        new_node->parent[0] = load_parent_info(load_leader());
        if (new_node->parent[0] == NULL) {
            xfree(new_node->parent);
            new_node->parent_num = 0;
        }
    } else {
        new_node->parent = NULL;
        new_node->parent_num = 0;
    }

    return new_node;
}

void CommitNodeFree(CommitNode **node) {
    xfree((*node)->log);
    xfree((*node)->datetime);
    xfree((*node)->commit_id);
    SnapshotBSTDestory(&((*node)->snapshot));
    for (size_t i = 0; i < (*node)->parent_num; i++) {
        CommitNodeFree(&((*node)->parent[i]));
    }
    xfree((*node)->parent);
    xfree((*node));
}

static char *log_file_handle() {
    FILE *log_file = xfopen(temp_log_filename, "rb");

    char c;
    while ((c = (char)fgetc(log_file)) != EOF && c != '\n');

    if ((c = (char)fgetc(log_file)) == EOF) {
        fclose(log_file);
        remove(temp_log_filename);
        ErrorCustomMsg("Commit operation cancelled\n");
    }

    size_t start_pos = (size_t)(ftell(log_file) - 1);
    fseek(log_file, 0, SEEK_END);
    size_t end_pos = (size_t)ftell(log_file);
    size_t content_length = end_pos - start_pos;
    char *log = xmalloc(content_length);

    fseek(log_file, (long)start_pos, SEEK_SET);
    fread(log, 1, content_length, log_file);
    log[content_length - 1] = '\0';

    fclose(log_file);
    return log;
}

static char *log_from_editor() {
    pid_t pid;

    pid = fork();

    if (pid == -1) {
        ErrorCustomMsg("Error: can not create child process\n");
    } else if (pid == 0) {
        char *argv[] = {"nano", (char *)temp_log_filename, NULL};
        FILE *temp_log_file = xfopen(temp_log_filename, "wb");
        fputs("// Write down your commit log below this line\n", temp_log_file);
        fclose(temp_log_file);
        execvp("nano", argv);
        ErrorCustomMsg("Error: can not open nano editor for commit log\n");
    } else {
        int status;
        wait(&status);
    }

    if (access(temp_log_filename, F_OK) == -1) {
        ErrorCustomMsg("Commit operation cancelled\n");
    }
    char *log = log_file_handle();

    remove(temp_log_filename);
    return log;
}

char *commit_log_insert(char *log_message) {
    if (log_message == NULL) {
        return log_from_editor();
    }
    return str_dup(log_message);
}

static void scan_and_create_snapshot(SnapshotNode *node) {
    FileInfo *current_file = get_fileinfo(node);
    mk_dir_and_file(current_file->path, current_file->content);
}

void save_object_file(CommitNode *node) {
    if (access(objects_dir, F_OK) == -1) {
        if (mkdir(objects_dir, 0775) == -1) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }
    }

    if (chdir(objects_dir) == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    char *pre_hash_string = xmalloc(strlen(node->log) + strlen(node->datetime) + 1);
    strcpy(pre_hash_string, node->log);
    strcat(pre_hash_string, node->datetime);

    char *object_dir = hash_to_string(hash_function(pre_hash_string));
    xfree(pre_hash_string);

    while (access(object_dir, F_OK) == 0) {
        char *temp_dir = str_dup(object_dir);
        xfree(object_dir);
        object_dir = hash_to_string(hash_function(temp_dir));
        xfree(temp_dir);
    }

    node->commit_id = str_dup(object_dir);

    if (mkdir(object_dir, 0775) == -1 || chdir(object_dir) == -1 || mkdir("root", 0775))
        ErrnoHandler(__func__, __FILE__, __LINE__);

    FILE *info_file = xfopen("info", "w");

    fprintf(info_file, "%s\n", node->datetime);
    if (node->parent != NULL) {
        fprintf(info_file, "%ld", node->parent_num);
        for (size_t i = 0; i < node->parent_num; i++) {
            fprintf(info_file, ", %s", node->parent[i]->commit_id);
        }
    }
    fputc('\n', info_file);
    fprintf(info_file, "%s\n", node->log);
    fclose(info_file);

    if (chdir("root") == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    inorder_traversal_func(node->snapshot, scan_and_create_snapshot);

    xfree(object_dir);
    cd_to_project_root(NULL);
}

void leader_update(CommitNode *node) {
    FILE *leader_file = xfopen(".big/Leader", "w");
    fprintf(leader_file, "%s\n", node->commit_id);
    fclose(leader_file);
}