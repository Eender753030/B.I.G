#include "commands/cmd_checkout.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/commit.h"
#include "core/index.h"
#include "core/snapshot.h"
#include "utils/error_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

static bool search_and_cmp_is_same(snapshot_bst_t *bst1, snapshot_bst_t *bst2) {
    uint64_t bst1_amount = bst_get_amount(bst1);
    uint64_t bst2_amount = bst_get_amount(bst2);

    if (bst1_amount != bst2_amount) {
        return false;
    }

    file_info_t **file_info_list = (file_info_t **)bst_inorder_to_list(bst2);

    for (uint64_t i = 0; i < bst2_amount; i++) {
        snapshot_node_t *target = bst_search(bst1, file_info_list[i]);
        if (target == NULL) {
            xfree(file_info_list);
            return false;
        }
        char *hash1, *hash2;

        file_info_t *temp_file_info = bst_node_get_data(target);
        file_info_get_content(temp_file_info, NULL, &hash1, NULL);
        file_info_get_content(file_info_list[i], NULL, &hash2, NULL);

        if (strcmp(hash1, hash2) != 0) {
            xfree(file_info_list);
            return false;
        }
    }

    xfree(file_info_list);

    return true;
}

void cmd_checkout(int argc, char *argv[]) {
    UNUSED(argv);

    if (check_init() == NOT_INIT) {
        error_not_init();
    }
    if (argc < 2) {
        error_custom_msg("Usage: big checkout <commit hash>\n");
    }

    cd_to_project_root(NULL);

    char *leader_hash = load_leader();
    if (leader_hash == NULL) {
        error_custom_msg("No commit\n");
    }
    char path[1024];
    snprintf(path, sizeof(path), ".big/objects/%s/list", leader_hash);
    xfree(leader_hash);

    snapshot_bst_t *dir_bst = snapshot_bst_create_from_projectdir();
    snapshot_bst_t *index_bst = read_index_file();
    snapshot_bst_t *leader_bst = read_index_file_from_path(path);

    if (is_same_bst(leader_bst, index_bst, is_same_file_info) == false ||
        search_and_cmp_is_same(index_bst, dir_bst) == false) {
        snapshot_bst_free(&leader_bst);
        snapshot_bst_free(&index_bst);
        snapshot_bst_free(&dir_bst);
        error_custom_msg(
            "Error: There are changes not commit. Please commit first or discard changes\n");
    }

    snapshot_bst_free(&leader_bst);
    snapshot_bst_free(&index_bst);
    snapshot_bst_free(&dir_bst);
}