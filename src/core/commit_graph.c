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
    FILE *leader = fopen(".big/Leader", "r");
    if (leader == NULL) {
        return NULL;
    }

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

CommitNode *load_parent_info(char *commit_id, long *limit_amount) {
    if (limit_amount != NULL && ((*limit_amount)--) <= 0) {
        xfree(commit_id);
        return NULL;
    }

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
        if (strncmp(buffer, "null\n", 6) != 0) {
            buffer[strcspn(buffer, "\n")] = '\0';
            char *parent_commit_id = str_dup(buffer);
            parent_node->parent = load_parent_info(parent_commit_id, limit_amount);
        } else {
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

    fclose(parent_info);

    return parent_node;
}

CommitNode *CommitNodeCreate(char *log) {
    CommitNode *new_node = xmalloc(sizeof(*new_node));

    char buffer[100];
    struct tm *datetime_now;
    time_t time_now = time(NULL);

    datetime_now = localtime(&time_now);
    strftime(buffer, 100, "%Y/%m/%d %H:%M:%S", datetime_now);

    while (strchr(log, '\n')) {
        log[strcspn(log, "\n")] = ' ';
    }
    new_node->log = log;
    new_node->datetime = str_dup(buffer);

    if (access(".big/Leader", F_OK) == 0) {
        long parent_num = 1;
        new_node->parent = load_parent_info(load_leader(), &parent_num);
        if (new_node->parent == NULL) {
            xfree(new_node->parent);
        }
    } else {
        new_node->parent = NULL;
    }

    return new_node;
}

void CommitNodeFree(CommitNode **node) {
    xfree((*node)->log);
    xfree((*node)->datetime);
    xfree((*node)->commit_id);
    while ((*node)->parent != NULL) {
        CommitNodeFree(&((*node)->parent));
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

static void scan_and_make(SnapshotNode *node) {
    char *path, *content;
    path_and_content_of_node(node, &path, &content);
    mk_dir_and_file(path, content);
}

void save_object_file(CommitNode *node) {
    if (mkdir(objects_dir, 0775) == -1) {
        if (errno != EEXIST) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }
    }

    char *list_file_content = read_whole_file(".big/index/index_list");
    char *leader_commit_id;
    SnapshotBST *bst_leader = read_leader_commit_BST(&leader_commit_id);
    SnapshotBST *cmp_snapshot = read_index_dic(NULL, NULL);

    if (bst_leader != NULL && is_same_tree(bst_leader, cmp_snapshot) == MATCH) {
        ErrorCustomMsg("Error: Nothing to commit\n");
    }
    SnapshotBSTDestory(&cmp_snapshot);

    if (chdir(objects_dir) == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    char *pre_hash_string = xmalloc(strlen(node->log) + strlen(node->datetime) + 1);
    strcpy(pre_hash_string, node->log);
    strcat(pre_hash_string, node->datetime);

    char *commit_dir = hash_to_string(hash_function(pre_hash_string));
    xfree(pre_hash_string);

    while (access(commit_dir, F_OK) == 0) {
        char *temp_dir = str_dup(commit_dir);
        xfree(commit_dir);
        commit_dir = hash_to_string(hash_function(temp_dir));
        xfree(temp_dir);
    }

    node->commit_id = str_dup(commit_dir);

    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "%s/root", commit_dir);
    if (mkdir(commit_dir, 0775) == -1 || chdir(commit_dir) == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    FILE *info_file = xfopen("info", "w");

    fprintf(info_file, "%s\n", node->datetime);
    if (node->parent != NULL) {
        fprintf(info_file, "%s\n", node->parent->commit_id);
    } else {
        fprintf(info_file, "%s\n", "null");
    }
    fprintf(info_file, "%s\n", node->log);
    fclose(info_file);

    FILE *list_file = xfopen("list", "w");
    fwrite(list_file_content, 1, strlen(list_file_content), list_file);
    fclose(list_file);

    if (mkdir("root", 0775) == -1 || chdir("root") == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    SnapshotBST *snapshot = read_index_dic(bst_leader, leader_commit_id);
    inorder_traversal_func(snapshot, scan_and_make);

    xfree(commit_dir);
    xfree(list_file_content);
    xfree(leader_commit_id);
    SnapshotBSTDestory(&bst_leader);
    SnapshotBSTDestory(&snapshot);
    cd_to_project_root(NULL);
}

void leader_update(CommitNode *node) {
    FILE *leader_file = xfopen(".big/Leader", "w");
    fprintf(leader_file, "%s\n", node->commit_id);
    fclose(leader_file);
}