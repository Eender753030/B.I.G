#include "core/commit.h"

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
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

struct commit_node {
    char *log;
    char *datetime;
    char *commit_hash;
    struct commit_node *parent;
};

char *load_leader() {
    FILE *leader = fopen(".big/Leader", "r");
    if (leader == NULL) {
        return NULL;
    }
    char branch_name[2048];
    if (fgets(branch_name, sizeof(branch_name), leader) == NULL) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    fclose(leader);
    branch_name[strcspn(branch_name, "\n")] = '\0';

    char ref_name[4096];
    snprintf(ref_name, sizeof(ref_name), ".big/refs/%s", branch_name);

    FILE *ref = fopen(ref_name, "r");
    uint64_t ref_hash_length = get_file_len(ref);
    if (ref_hash_length == 0) {
        fclose(ref);
        return NULL;
    }

    char *ref_hash = xmalloc(ref_hash_length + 1);

    uint64_t read_bytes = fread(ref_hash, 1, ref_hash_length, ref);
    fclose(ref);
    if (read_bytes != ref_hash_length) {
        return NULL;
    }

    ref_hash[ref_hash_length] = '\0';

    if (ref_hash[ref_hash_length - 1] == '\n') {
        ref_hash[ref_hash_length - 1] = '\0';
    }
    return ref_hash;
}

commit_node_t *load_parent_info(char *commit_id, long *limit_amount) {
    if (limit_amount != NULL && ((*limit_amount)--) <= 0) {
        xfree(commit_id);
        return NULL;
    }

    commit_node_t *parent_node = xmalloc(sizeof(*parent_node));

    parent_node->commit_hash = commit_id;

    char parent_obj[1024];
    snprintf(parent_obj, 1024, ".big/objects/%s/info", commit_id);
    FILE *parent_info = xfopen(parent_obj, "r");

    char buffer[128] = {0};

    fgets(buffer, 128, parent_info);
    buffer[strcspn(buffer, "\n")] = '\0';
    parent_node->datetime = str_dup(buffer);

    if (fgets(buffer, 128, parent_info) != NULL) {
        if (strcmp(buffer, "null\n") != 0) {
            buffer[strcspn(buffer, "\n")] = '\0';
            char *parent_commit_id = str_dup(buffer);
            parent_node->parent = load_parent_info(parent_commit_id, limit_amount);
        } else {
            parent_node->parent = NULL;
        }
    }

    uint64_t log_length = get_file_len(parent_info) - 1;
    parent_node->log = xmalloc(log_length + 1);

    fread(parent_node->log, 1, log_length, parent_info);
    parent_node->log[log_length] = '\0';

    fclose(parent_info);

    return parent_node;
}

static char *log_file_handle() {
    FILE *log_file = xfopen(".big/temp.txt", "rb");

    char c;
    while ((c = (char)fgetc(log_file)) != EOF && c != '\n');

    if ((c = (char)fgetc(log_file)) == EOF) {
        fclose(log_file);
        remove(".big/temp.txt");
        error_custom_msg("Commit operation cancelled\n");
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
        error_custom_msg("Error: can not create child process\n");
    } else if (pid == 0) {
        char *argv[] = {"nano", (char *)".big/temp.txt", NULL};
        FILE *temp_log_file = xfopen(".big/temp.txt", "wb");
        fputs("// Write down your commit log below this line\n", temp_log_file);
        fclose(temp_log_file);
        execvp("nano", argv);
        error_custom_msg("Error: can not open nano editor for commit log\n");
    } else {
        int status;
        wait(&status);
    }

    if (access(".big/temp.txt", F_OK) == -1) {
        error_custom_msg("Commit operation cancelled\n");
    }
    char *log = log_file_handle();

    remove(".big/temp.txt");
    return log;
}

static char *commit_log_insert(const char *log_message) {
    char *new_log;
    if (log_message == NULL) {
        new_log = log_from_editor();
    } else {
        new_log = str_dup(log_message);
    }

    for (char *c = new_log; *c != '\0'; c++) {
        if (*c == '\n') {
            *c = ' ';
        }
    }

    return new_log;
}

commit_node_t *commit_node_create(const char *log) {
    commit_node_t *new_node = xmalloc(sizeof(*new_node));

    new_node->log = commit_log_insert(log);
    new_node->datetime = datetime_now_to_str();

    if (access(".big/Leader", F_OK) == 0) {
        long parent_num = 1;
        new_node->parent = load_parent_info(load_leader(), &parent_num);
        if (new_node->parent == NULL) {
            xfree(new_node->parent);
        }
    } else {
        new_node->parent = NULL;
    }

    char parent_id[128] = "null";
    char buffer[4096];
    if (new_node->parent != NULL) {
        strcpy(parent_id, new_node->parent->commit_hash);
    }

    snprintf(buffer, sizeof(buffer), "%s%s%s", log, new_node->datetime, parent_id);
    new_node->commit_hash = hash_to_string(hash_function(buffer));

    return new_node;
}

void commit_node_free(commit_node_t **node) {
    if (node == NULL || *node == NULL) {
        return;
    }

    xfree((*node)->log);
    xfree((*node)->datetime);
    xfree((*node)->commit_hash);
    if ((*node)->parent != NULL) {
        commit_node_free(&((*node)->parent));
    }
    xfree((*node));
}

void save_commit_obj(commit_node_t *node) {
    char commit_dir[2048] = {0};
    snprintf(commit_dir, sizeof(commit_dir), ".big/objects/%s", node->commit_hash);
    if (mkdir(commit_dir, 0775) == -1) {
        errno_handle(__func__, __FILE__, __LINE__);
    }

    uint64_t content_len;
    char *index_content = read_whole_file(".big/index", &content_len);

    char buffer[4096] = {0};
    snprintf(buffer, sizeof(buffer), "%s/list", commit_dir);

    FILE *list_file = xfopen(buffer, "wb");
    uint64_t write_bytes = fwrite(index_content, 1, content_len, list_file);
    if (write_bytes != content_len) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    fclose(list_file);

    snprintf(buffer, sizeof(buffer), "%s/info", commit_dir);

    FILE *info_file = xfopen(buffer, "w");
    fprintf(info_file, "%s\n", node->datetime);
    if (node->parent != NULL) {
        fprintf(info_file, "%s\n", node->parent->commit_hash);
    } else {
        fprintf(info_file, "%s\n", "null");
    }
    fprintf(info_file, "%s\n", node->log);
    fclose(info_file);

    xfree(index_content);
}

void update_branch_with_hash(const char *hash) {
    if (access(".big/leader", F_OK) != 0) {
        FILE *leader_file = xfopen(".big/Leader", "w");
        if (fputs("main", leader_file) == EOF) {
            errno_handle(__func__, __FILE__, __LINE__);
        }
        fclose(leader_file);
    }

    FILE *leader_file = xfopen(".big/Leader", "r");
    char branch_name[2048];
    if (fgets(branch_name, sizeof(branch_name), leader_file) == NULL) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    fclose(leader_file);

    branch_name[strcspn(branch_name, "\n")] = '\0';

    char ref_name[4096];
    snprintf(ref_name, sizeof(ref_name), ".big/refs/%s", branch_name);
    FILE *ref_file = xfopen(ref_name, "w");
    fprintf(ref_file, "%s\n", hash);
    fclose(ref_file);
}

void update_branch(commit_node_t *node) {
    update_branch_with_hash(node->commit_hash);
}

char *load_current_branch() {
    FILE *leader_file = fopen(".big/Leader", "r");
    if (leader_file == NULL) {
        return NULL;
    }

    uint64_t branch_name_length = get_file_len(leader_file);
    if (branch_name_length == 0) {
        return NULL;
    }

    char *branch_name = xmalloc(branch_name_length + 1);

    uint64_t read_bytes = fread(branch_name, 1, branch_name_length, leader_file);
    fclose(leader_file);
    if (read_bytes != branch_name_length) {
        return NULL;
    }

    branch_name[branch_name_length] = '\0';

    if (branch_name[branch_name_length - 1] == '\n') {
        branch_name[branch_name_length - 1] = '\0';
    }

    return branch_name;
}

void get_commit_node_info(commit_node_t *node, char **log, char **datetime, char **hash) {
    if (node == NULL) {
        return;
    }
    if (log != NULL) {
        *log = node->log;
    }
    if (datetime != NULL) {
        *datetime = node->datetime;
    }
    if (hash != NULL) {
        *hash = node->commit_hash;
    }
}

commit_node_t *get_commit_parent(commit_node_t *node) {
    if (node == NULL) {
        return NULL;
    }
    return node->parent;
}