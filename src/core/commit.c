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

// commit_node structure for every commit
struct commit_node {
    char *log;                   // log in commit
    char *datetime;              // commit created time
    char *commit_hash;           // Every commit has different unique hash
    struct commit_node *parent;  // point to parent commit
};

char *load_leader() {
    FILE *leader = fopen(".big/Leader", "r");
    if (leader == NULL) {
        return NULL;
    }
    char branch_name[2048];
    // Get current branch name from Leader(HEAD) file
    if (fgets(branch_name, sizeof(branch_name), leader) == NULL) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    fclose(leader);
    // Replace the '\n' to '\0'
    branch_name[strcspn(branch_name, "\n")] = '\0';

    char ref_name[4096];
    // Combine the path to branch
    snprintf(ref_name, sizeof(ref_name), ".big/refs/%s", branch_name);

    FILE *ref = fopen(ref_name, "r");
    uint64_t ref_hash_length = get_file_len(ref);
    if (ref_hash_length == 0) {
        fclose(ref);
        return NULL;
    }

    char *ref_hash = xmalloc(ref_hash_length + 1);
    // Read newest commit hash from branch
    uint64_t read_bytes = fread(ref_hash, 1, ref_hash_length, ref);
    fclose(ref);
    if (read_bytes != ref_hash_length) {
        return NULL;
    }

    ref_hash[ref_hash_length] = '\0';
    // Double check for ensure '\0'
    if (ref_hash[ref_hash_length - 1] == '\n') {
        ref_hash[ref_hash_length - 1] = '\0';
    }
    return ref_hash;
}

// limit_amount for prevent load all parents of commit
commit_node_t *load_parent_info(char *commit_id, long *limit_amount) {
    if (limit_amount != NULL && ((*limit_amount)--) <= 0) {
        xfree(commit_id);
        return NULL;
    }

    commit_node_t *parent_node = xmalloc(sizeof(*parent_node));

    parent_node->commit_hash = commit_id;

    char parent_obj[1024];
    // Combine the path to parent metadata
    snprintf(parent_obj, 1024, ".big/objects/%s/info", commit_id);
    FILE *parent_info = xfopen(parent_obj, "r");

    char buffer[128] = {0};

    // Get parent node's datetime
    if (fgets(buffer, 128, parent_info) == NULL) {
        fclose(parent_info);
        xfree(parent_node);
        xfree(commit_id);
        return NULL;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
    parent_node->datetime = str_dup(buffer);

    // Get parent's parent
    if (fgets(buffer, 128, parent_info) != NULL) {
        // if the string is not "null\n" means it has parent
        if (strcmp(buffer, "null\n") != 0) {
            buffer[strcspn(buffer, "\n")] = '\0';
            char *parent_commit_id = str_dup(buffer);
            // Continue to get parent
            parent_node->parent = load_parent_info(parent_commit_id, limit_amount);
        } else {
            parent_node->parent = NULL;
        }
    }

    uint64_t log_length = get_file_len(parent_info) - 1;
    parent_node->log = xmalloc(log_length + 1);

    // Read last file content that is log
    uint64_t read_bytes = fread(parent_node->log, 1, log_length, parent_info);
    if (read_bytes != log_length) {
        fclose(parent_info);
        commit_node_free(&parent_node);
        return NULL;
    }

    parent_node->log[log_length] = '\0';

    fclose(parent_info);

    return parent_node;
}

// helper function for read log from temp file create from nano(editor)
static char *log_file_handle() {
    FILE *log_file = xfopen(".big/temp.txt", "rb");

    char c;
    // First line is hint so skip it
    while ((c = (char)fgetc(log_file)) != EOF && c != '\n')
        ;
    // If Second line reach to end of file means no input log, cancel 'commit' command
    if ((c = (char)fgetc(log_file)) == EOF) {
        fclose(log_file);
        remove(".big/temp.txt");
        error_custom_msg("Commit operation cancelled\n");
    }
    // Move back two head of second line because of the check
    fseek(log_file, -1, SEEK_CUR);

    uint64_t content_length = get_file_len(log_file);

    char *log = xmalloc(content_length);

    // Read all content below first line
    uint64_t read_bytes = fread(log, 1, content_length, log_file);
    if (read_bytes != content_length) {
        fclose(log_file);
        xfree(log);
        error_custom_msg("Commit operation cancelled\n");
    }
    log[content_length - 1] = '\0';

    fclose(log_file);
    return log;
}

// Helper function if use editor to write log
static char *log_from_editor() {
    pid_t pid;

    // Use fork to create child process to execute nano(editor)
    pid = fork();

    if (pid == -1) {
        error_custom_msg("Error: can not create child process\n");
    } else if (pid == 0) {
        // Child fork to execute nano(editor)
        char *argv[] = {"nano", ".big/temp.txt", NULL};
        // Create a temp file and write hint into it
        FILE *temp_log_file = xfopen(".big/temp.txt", "wb");
        fputs("// Write down your commit log below this line\n", temp_log_file);
        fclose(temp_log_file);
        // execute nano .big/temp.txt;
        execvp("nano", argv);  // Child end when nano close and get kill
        error_custom_msg("Error: can not open nano editor for commit log\n");
    } else {
        // wait for child
        int status;
        wait(&status);
    }

    if (access(".big/temp.txt", F_OK) == -1) {
        error_custom_msg("Commit operation cancelled\n");
    }
    char *log = log_file_handle();  // Get log from helper

    remove(".big/temp.txt");  // Remove temp file
    return log;
}

// Helper for parse user command
static char *commit_log_insert(const char *log_message) {
    char *new_log;
    if (log_message == NULL) {
        new_log = log_from_editor();
    } else {
        new_log = str_dup(log_message);
    }

    // Change all '\n' to space
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

    // Check there has any commit exist
    if (access(".big/Leader", F_OK) == 0) {
        long parent_num = 1;
        // Just need a parent for write metadata
        new_node->parent = load_parent_info(load_leader(), &parent_num);
    } else {
        new_node->parent = NULL;
    }

    char parent_id[128] = "null";
    char buffer[4096];
    if (new_node->parent != NULL) {
        strcpy(parent_id, new_node->parent->commit_hash);
    }

    // Using log, datetime, and parent's hash to create hash make sure no collision
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
    // Recurrsive free parent node
    if ((*node)->parent != NULL) {
        commit_node_free(&((*node)->parent));
    }
    xfree((*node));
}

/* The Implicit Graph
 * Here is where the Implicit Graph start formation
 * Every node simply record its parent
 */
void save_commit_obj(commit_node_t *node) {
    char commit_dir[2048] = {0};
    // Combine path of commit object
    snprintf(commit_dir, sizeof(commit_dir), ".big/objects/%s", node->commit_hash);
    if (mkdir(commit_dir, 0775) == -1) {
        errno_handle(__func__, __FILE__, __LINE__);
    }

    uint64_t content_len;
    // Get all content in index
    char *index_content = read_whole_file(".big/index", &content_len);

    char buffer[4096] = {0};
    // Combine path of commit index list
    snprintf(buffer, sizeof(buffer), "%s/list", commit_dir);

    // The metadata
    FILE *list_file = xfopen(buffer, "wb");
    // Write whole index file to the list
    uint64_t write_bytes = fwrite(index_content, 1, content_len, list_file);
    if (write_bytes != content_len) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    fclose(list_file);

    // Combine path of commit metadata file
    snprintf(buffer, sizeof(buffer), "%s/info", commit_dir);

    FILE *info_file = xfopen(buffer, "w");
    // First line is datetime
    fprintf(info_file, "%s\n", node->datetime);
    // Second line is for parent, if no parent, write null instead
    // Here we create the DAG's edge
    if (node->parent != NULL) {
        fprintf(info_file, "%s\n", node->parent->commit_hash);
    } else {
        fprintf(info_file, "%s\n", "null");
    }
    // Write whole log into it
    fprintf(info_file, "%s\n", node->log);
    fclose(info_file);

    xfree(index_content);
}

void update_leader(const char *branch_name) {
    // Use write mode to replace old content with new branch that point to
    FILE *leader_file = xfopen(".big/Leader", "w");
    if (fprintf(leader_file, "%s\n", branch_name) < 0) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    fclose(leader_file);
}

void update_branch_with_hash(const char *hash) {
    // If Leader not exist, create one and point to default branch "main"
    if (access(".big/Leader", F_OK) != 0) {
        FILE *leader_file = xfopen(".big/Leader", "w");
        if (fputs("main\n", leader_file) == EOF) {
            errno_handle(__func__, __FILE__, __LINE__);
        }
        fclose(leader_file);
    }

    // Read out the branch name inside Leader
    FILE *leader_file = xfopen(".big/Leader", "r");
    char branch_name[2048];
    if (fgets(branch_name, sizeof(branch_name), leader_file) == NULL) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    fclose(leader_file);

    branch_name[strcspn(branch_name, "\n")] = '\0';

    // Write new hash into current branch
    char ref_name[4096];
    snprintf(ref_name, sizeof(ref_name), ".big/refs/%s", branch_name);
    FILE *ref_file = xfopen(ref_name, "w");
    fprintf(ref_file, "%s\n", hash);
    fclose(ref_file);
}

void update_branch(commit_node_t *node) {
    // Use commit node's hash for 'commit' command work
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

char *load_ref_hash(const char *path) {
    FILE *ref_file = fopen(path, "r");
    if (ref_file == NULL) {
        return NULL;
    }

    uint64_t hash_length = get_file_len(ref_file);
    if (hash_length == 0) {
        return NULL;
    }

    char *hash = xmalloc(hash_length + 1);

    uint64_t read_bytes = fread(hash, 1, hash_length, ref_file);
    fclose(ref_file);
    if (read_bytes != hash_length) {
        return NULL;
    }

    hash[hash_length] = '\0';

    if (hash[hash_length - 1] == '\n') {
        hash[hash_length - 1] = '\0';
    }

    return hash;
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