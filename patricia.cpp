/*
 * patricia.c
 *
 * This file implements the PATRICIA tree datastructure, also known as the
 * RADIX tree. The purpose of the patricia tree in this project is to store
 * millions of strings efficiently and do prefix based lookups. Hence there
 * is no data associated with the individual nodes. Implementation is based
 * on the algorithm given in http://en.wikipedia.org/wiki/Radix_tree
 *
 * Dileep Ramesh, July 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "patricia.h"

#ifdef PATRICIA_STATS_ON
/*
 * Globals
 */
static patricia_stats_t stats;
#endif

/*
 * substring
 *
 * Utility function to return a substring of the given string. Note that
 * strndup allocates memory for the new string. The caller has to free it.
 */
char *
substring (const char *str, int begin, int len) 
{
    char *substr;
    int i;

    /* Sanity check */
    if (str == 0 || strlen(str) == 0 || strlen(str) < begin || 
        strlen(str) < (begin+len)) { 
        return 0;
    }

    substr = (char *)malloc(len + 1);
#ifdef PATRICIA_STATS_ON
    stats.total_mem += (len + 1);
#endif

    i = 0;
    while (i < len) {
        substr[i] = str[begin + i];
        i++;
    }
    substr[i] = 0;

    return substr; 
}

/*
 * is_substring
 *
 * Utility function which checks if str2 is a substring of str1
 */
static int
is_substring (char *str1, char *str2)
{
    int i, j, len1, len2, count, found = 0;

    len1 = strlen(str1);
    len2 = strlen(str2);

    if (len1 < len2) {
        return 0;
    }

    for (i = 0; i < len1; i++) {
        j = 0;
        count = 0;
        while (str1[i] != ' ') {
            if (str1[i++] == str2[j++]) {
                count++;
            } else {
                break;
            }
        }
        if (count == len2) {
            found = 1;
            break;
        }
    }

    if (found) {
        return 1;
    }

    return 0;
}

/*
 * patricia_node_init
 *
 * Create a new node for the given key
 */
patricia_node_t *
patricia_node_init (char *key, uint8_t create_list)
{
    int ret = 0, keylen;
    patricia_node_t *node;

    /* Sanity check */
    if (!key) {
        return NULL;
    }

    /* Create the node */
    node = (patricia_node_t *)malloc(sizeof(patricia_node_t));
    if (!node) {
        return NULL;
    }
#ifdef PATRICIA_STATS_ON
    stats.total_mem += sizeof(patricia_node_t);
    stats.total_nodes++;
#endif

    keylen = strlen(key);
    node->key = (char *)malloc(keylen + 1);
    if (!node->key) {
        return NULL;
    }
    strncpy(node->key, key, keylen);
    node->key[keylen] = 0;
#ifdef PATRICIA_STATS_ON
    stats.total_mem += (keylen + 1);
#endif

    /*
     * Are we asked to create a children list? Will be false in case of a node
     * split.
     */
    if (create_list) {
        node->children = list_create();
        if (!node->children) {
            return NULL;
        }
#ifdef PATRICIA_STATS_ON
        stats.total_mem += sizeof(list_t);
#endif
    }

    return node;
}

/*
 * patricia_add_child_node
 *
 * Add the given child node to the parent's list of children
 */
static void
patricia_add_child_node (patricia_node_t *parent, patricia_node_t *child)
{
    int ret = 0;
    patricia_node_t *node, *next_node;

    /* Sanity check */
    if (!parent || !child) {
        return;
    }

    /* 
     * Insert it at the right place. We need to maintain lexicographical 
     * ordering 
     */
    if (list_empty(parent->children)) {
        list_insert(parent->children, &child->link);
        return;
    }

    node = (patricia_node_t *)list_get_head(parent->children);
    while (node) {
        next_node = (patricia_node_t *)list_get_next(parent->children, node);
        if (strcmp(child->key, node->key) < 0) {
            list_insert_before(parent->children, &node->link, 
                               &child->link);
            return;
        }
        node = next_node;
    }

    list_insert(parent->children, &child->link);
}

/*
 * patricia_get_prefix_count
 *
 * This routine takes two strings and returns the length of the common prefix
 */
static int
patricia_get_prefix_count (char *key1, char *key2)
{
    int len1, len2, len, prefix_count, i;

    /* Sanity check */
    if (!key1 || !key2) {
        return 0;
    }

    /* Compute the common prefix */
    len1 = strlen(key1);
    len2 = strlen(key2);
    prefix_count = 0;

    if (len1 == 0 || len2 == 0) {
        return 0;
    }

    len = (len1 > len2) ? len2 : len1;

    for (i = 0; i <len; i++) {
        if (key1[i] == key2[i]) {
            prefix_count++;
        } else {
            break;
        }
    }

    return prefix_count;
}

/*
 * patricia_get_key_count
 *
 * This function recursively computes the total number of keys stored in the
 * given tree. The result in placed in count.
 */
void
patricia_get_key_count (patricia_node_t *root, unsigned long *count)
{
#ifdef PATRICIA_STATS_ON
    patricia_node_t *child, *next_child;

    /* Sanity check */
    if (!root) {
        return;
    }

    child = (patricia_node_t *)list_get_head(root->children);
    while (child) {
        next_child = (patricia_node_t *)list_get_next(root->children, child);
        patricia_get_key_count(child, count);
        child = next_child;
    }

    /* Update the count when we reach a leaf node */
    if (list_empty(root->children)) {
        *count = *count + 1;
    }
#endif
}

/*
 * patricia_print_stats
 *
 * Dump the stats for the given tree
 */
void
patricia_print_stats (patricia_tree_t *tree)
{
#ifdef PATRICIA_STATS_ON
    stats.total_keys = 0;
    patricia_get_key_count(tree->root, &stats.total_keys);

    printf("\nTotal number of keys: %lu\n", stats.total_keys);
    printf("Total number of nodes: %lu\n", stats.total_nodes);
    printf("Total memory used: %lu bytes\n\n", stats.total_mem);
#endif
}

/*
 * patricia_key_has_delimiter
 *
 * This function checks whether the given node has the '/' directory delimiter
 */
int
patricia_key_has_delimiter (char *key)
{
    int len;

    if (!key) {
        return 0;
    }

    len = strlen(key);
    if (key[len - 1] == '/') {
        return 1;
    }

    return 0;
}

/*
 * patricia_lookup_node_internal
 *
 * This is a recursive routine which looks up a given key and returns the
 * corresponding node in the patricia tree. Returns NULL if key is not present
 * in the tree.
 */
static patricia_node_t *
patricia_lookup_node_internal (patricia_tree_t *tree, 
                               patricia_node_t *cur_node,
                               char *key)
{
    int prefix_len;
    char *new_key;
    patricia_node_t *child, *next_child, *retnode;

    /* Sanity check */
    if (!tree || !cur_node || !key) {
        return NULL;
    }

    /* 
     * Get the length of the longest common prefix between the given key and
     * the key stored in the current node
     */
    prefix_len = patricia_get_prefix_count(key, cur_node->key);

    /*
     * We have 4 cases:
     *
     * 1. prefix_len = 0
     * 2. prefix_len < len(key), prefix_len >= len(cur_node->key)
     * 3. prefix_len = len(cur_node->key)
     * 4. cur_node = root
     *
     * For cases (1), (2) and (4), update the search key by removing the
     * common prefix and search for the node with this new key among the 
     * list of children.
     *
     * We hit case (3) when we get the node being requested for.
     */
    if (cur_node == tree->root || prefix_len == 0 ||
        (prefix_len > 0 && 
         prefix_len < strlen(key) && 
         prefix_len >= strlen(cur_node->key))) {

        new_key = substring(key, prefix_len, strlen(key) - prefix_len);
        if (!new_key) {
            return NULL;
        }

        child = (patricia_node_t *)list_get_head(cur_node->children);
        while (child) {
            next_child = (patricia_node_t *)list_get_next(cur_node->children, child);
            if (child->key[0] == new_key[0]) {
                retnode = patricia_lookup_node_internal(tree, child, new_key);
#ifdef PATRICIA_STATS_ON
                stats.total_mem -= strlen(new_key);
#endif
                free(new_key);
                return retnode;
            }
            child = next_child;
        }

        if (new_key) {
#ifdef PATRICIA_STATS_ON
            stats.total_mem -= strlen(new_key);
#endif
            free(new_key);
        }

        return NULL;
    } else if (prefix_len == strlen(cur_node->key)) {
        return cur_node;
    } else {
        return NULL;
    }
}

/*
 * patricia_lookup_node
 *
 * This function takes a key and returns the corresponding patricia tree node.
 * Returns NULL if key is not found.
 */
static patricia_node_t *
patricia_lookup_node (patricia_tree_t *tree, char *key)
{
    return patricia_lookup_node_internal(tree, tree->root, key);
}

/*
 * patricia_lookup_internal
 *
 * This is a recursive routine which looks up a given key in the patricia
 * tree. Returns 1 if found, 0 otherwise.
 */
static int
patricia_lookup_internal (patricia_tree_t *tree, patricia_node_t *cur_node, 
                          char *key)
{
    int prefix_len, ret = 0;
    char *new_key;
    patricia_node_t *child, *next_child;

    /* Sanity check */
    if (!tree || !cur_node || !key) {
        return 0;
    }

    /* 
     * Get the length of the longest common prefix between the given key and
     * the key stored in the current node
     */
    prefix_len = patricia_get_prefix_count(key, cur_node->key);

    /*
     * We have 4 cases:
     *
     * 1. prefix_len = 0
     * 2. prefix_len < len(key), prefix_len >= len(cur_node->key)
     * 3. prefix_len = len(cur_node->key)
     * 4. cur_node = root
     *
     * For cases (1), (2) and (4), update the search key by removing the
     * common prefix and search for the node with this new key among the 
     * list of children.
     *
     * We hit case (3) when we find the key being looked up.
     */
    if (cur_node == tree->root || prefix_len == 0 ||
        (prefix_len > 0 && 
         prefix_len < strlen(key) && 
         prefix_len >= strlen(cur_node->key))) {

        new_key = substring(key, prefix_len, strlen(key) - prefix_len);
        if (!new_key) {
            return 0;
        }

        child = (patricia_node_t *)list_get_head(cur_node->children);
        while (child) {
            next_child = (patricia_node_t *)list_get_next(cur_node->children, child);
            if (child->key[0] == new_key[0]) {
                ret = patricia_lookup_internal(tree, child, new_key);
#ifdef PATRICIA_STATS_ON
                stats.total_mem -= strlen(new_key);
#endif
                free(new_key);
                return ret;
            }
            child = next_child;
        }

        if (new_key) {
#ifdef PATRICIA_STATS_ON
            stats.total_mem -= strlen(new_key);
#endif
            free(new_key);
        }

        return 0;
    } else if (prefix_len == strlen(cur_node->key)) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * patricia_lookup
 *
 * This function takes a key and searches it in the given patricia tree.
 * Returns 1 if found, 0 otherwise.
 */
int
patricia_lookup (patricia_tree_t *tree, char *key)
{
    return patricia_lookup_internal(tree, tree->root, key);
}

/*
 * patricia_lookup_prefix_partial_internal
 *
 * This is a recursive routine which retrieves all the keys sharing the given
 * prefix. The result is placed in res_list supplied by the caller.
 */
static void
patricia_lookup_prefix_partial_internal (patricia_node_t *cur_node, char *res,
                                         char *res_list, char *prefix)
{
    patricia_node_t *child, *next_child;
    char prev_res[PATRICIA_DEFAULT_KEYLEN];
    char temp_res[PATRICIA_DEFAULT_KEYLEN];
    int len;

    /* Sanity check */
    if (!cur_node || !prefix || !res || !res_list) {
        return;
    }

    /* This is similar to a depth first search of the tree */
    child = (patricia_node_t *)list_get_head(cur_node->children);
    while (child) {
        next_child = (patricia_node_t *)list_get_next(cur_node->children, child);

        /* Store the result. Will be needed if we make a recursive call */
        len = strlen(res);
        if (len > 0) {
            strncpy(prev_res, res, len);
            prev_res[len] = 0;
        } else {
            prev_res[0] = 0;
        }
        strcat(res, child->key);

        /* 
         * Check if we have to continue down the path or stop and move on to
         * the next child 
         */
        if (patricia_key_has_delimiter(child->key)) {
            res[strlen(res) - 1] = 0;
            /* 
             * TODO Hack to ensure duplicates are not added to the list. See if we
             * can avoid this!
             */
            temp_res[0] = 0;
            strncpy(temp_res, prefix, strlen(prefix));
            temp_res[strlen(prefix)] = 0;
            strcat(temp_res, res);

            if (!is_substring(res_list, temp_res)) {
                strcat(res_list, prefix);
                strcat(res_list, res);
                strcat(res_list, " ");
            }
            res[0] = 0;
        } else {
            patricia_lookup_prefix_partial_internal(child, res, res_list, prefix);
        }

        /* Restore the saved value */
        if (len > 0) {
            strncpy(res, prev_res, len);
            res[len] = 0;
        } else {
            res[0] = 0;
        }

        child = next_child;
    }

    /* Update res_list only when we reach a leaf node */
    if (list_empty(cur_node->children)) {
        /* 
         * TODO Hack to ensure duplicates are not added to the list. See if we
         * can avoid this!
         */
        temp_res[0] = 0;
        strncpy(temp_res, prefix, strlen(prefix));
        temp_res[strlen(prefix)] = 0;
        strcat(temp_res, res);

        if (!is_substring(res_list, temp_res)) {
            strcat(res_list, prefix);
            strcat(res_list, res);
            strcat(res_list, " ");
        }
    }
}

/*
 * patricia_lookup_prefix_partial
 *
 * This routine takes a prefix and returns all the keys sharing that prefix.
 * The result is placed in res_list supplied by the caller.
 */
int
patricia_lookup_prefix_partial (patricia_tree_t *tree, char *prefix, 
                                char *res_list)
{
    patricia_node_t *prefix_node;
    char res[PATRICIA_DEFAULT_KEYLEN];
    int prefix_len;

    /* Sanity check */
    if (!tree || !prefix || !res_list) {
        return -1;
    }
    prefix_len = strlen(prefix);

    /* 
     * First get the node corresponding to the passed prefix. Search starts
     * from there.
     */
    prefix_node = patricia_lookup_node(tree, prefix);
    if (!prefix_node) {
        return -1;
    }
    res[0] = 0;

    patricia_lookup_prefix_partial_internal(prefix_node, res, res_list, prefix);

    return 0;
}

/*
 * patricia_lookup_prefix_full_internal
 *
 * This is a recursive routine which retrieves all the keys sharing the given
 * prefix. The result is stored in res_list.
 */
static void
patricia_lookup_prefix_full_internal (patricia_node_t *cur_node,
                                      char *res, char *res_list)
{
    patricia_node_t *child, *next_child;
    int len;
    char prev_res[PATRICIA_DEFAULT_KEYLEN];

    /* Sanity check */
    if (!cur_node || !res || !res_list) {
        return;
    }

    len = strlen(res);
    strncpy(prev_res, res, len);
    prev_res[len] = 0;

    strcat(res, cur_node->key);    
    child = (patricia_node_t *)list_get_head(cur_node->children);
    while (child) {
        next_child = (patricia_node_t *)list_get_next(cur_node->children, child);
        patricia_lookup_prefix_full_internal(child, res, res_list);
        child = next_child;
    }

    if (list_empty(cur_node->children)) {
        strcat(res_list, res);
        strcat(res_list, " ");
    }

    strncpy(res, prev_res, len);
    res[len] = 0;
}

/*
 * patricia_lookup_prefix_full
 *
 * This routine takes a prefix and returns all the keys sharing that prefix
 */
int
patricia_lookup_prefix_full (patricia_tree_t *tree, char *prefix, char *buf)
{
    patricia_node_t *prefix_node;
    char res[PATRICIA_DEFAULT_KEYLEN];
    int keylen;

    /* Sanity check */
    if (!tree || !prefix) {
        return -1;
    }

    prefix_node = patricia_lookup_node(tree, prefix);
    if (!prefix_node) {
        return -1;
    }

    keylen = strstr(prefix, prefix_node->key) - (char *)prefix;
    strncpy(res, prefix, keylen);
    res[keylen] = 0;

    patricia_lookup_prefix_full_internal(prefix_node, res, buf);

    return 0;
}

/*
 * patricia_delete_keys
 *
 * Recursively delete all the keys under the given node and free the
 * associated memory
 */
static int
patricia_delete_keys (patricia_node_t *root)
{
    patricia_node_t *child, *next_child;
#ifdef PATRICIA_STATS_ON
    int len = strlen(root->key);
#endif

    /* Sanity check */
    if (!root) {
        return -1;
    }

    /* Do a depth first search */
    child = (patricia_node_t *)list_get_head(root->children);
    while (child) {
        next_child = (patricia_node_t *)list_get_next(root->children, child);
        list_remove(root->children, &child->link);
        patricia_delete_keys(child);
        child = next_child;
    }

    /* We have cleaned up all the children. Its safe to blow away this node. */
    list_destroy(root->children);
    free(root->key);
    free(root);
#ifdef PATRICIA_STATS_ON
    stats.total_mem -= (sizeof(list_t) + len + sizeof(patricia_node_t));
    stats.total_nodes--;
#endif

    return 0;
}

/*
 * patricia_delete_internal
 *
 * Recursive routine which removes a key from the given patricia tree. If the
 * key is not associated with a leaf, all the keys having the given key as
 * prefix will be removed.
 */
static int
patricia_delete_internal (patricia_tree_t *tree, patricia_node_t *cur_node, 
                          char *key)
{
    int prefix_len, ret = 0;
    char *new_key, deleted = 0;
    patricia_node_t *child, *next_child;

    /* Sanity check */
    if (!tree || !cur_node || !key) {
        return 0;
    }

    /* 
     * Get the length of the longest common prefix between the given key and
     * the key stored in the current node
     */
    prefix_len = patricia_get_prefix_count(key, cur_node->key);

    /*
     * We check for 3 cases:
     *
     * 1. prefix_len = 0
     * 2. prefix_len < len(key), prefix_len >= len(cur_node->key)
     * 3. cur_node = root
     *
     * If (1), (2) or (3) is true, we update the key by removing the prefix
     * key[0..prefix_len] and searching the next level of children for a
     * match. Once we come to the node containing the search key, we
     * try to delete and free everything under it.
     */
    if (cur_node == tree->root || prefix_len == 0 ||
        (prefix_len > 0 && 
         prefix_len < strlen(key) && 
         prefix_len >= strlen(cur_node->key))) {

        new_key = substring(key, prefix_len, strlen(key) - prefix_len);
        if (!new_key) {
            return -1;
        }

        child = (patricia_node_t *)list_get_head(cur_node->children);
        while (child) {
            next_child = (patricia_node_t *)list_get_next(cur_node->children, child);
            if (child->key[0] == new_key[0]) {
                if (strcmp(child->key, new_key) == 0) {
                    ret = list_remove(cur_node->children, &child->link);
                    if (ret != 0) {
                        break;
                    }

                    ret = patricia_delete_keys(child);
                    if (ret != 0) {
                        break;
                    }

                    deleted = 1;
                    break;
                }
                return patricia_delete_internal(tree, child, new_key);
            }
            child = next_child;
        }

        if (new_key) {
#ifdef PATRICIA_STATS_ON
            stats.total_mem -= strlen(new_key);
#endif
            free(new_key);
        }
    }

    /* Return the appropriate status back to the caller */
    if (deleted) {
        return 0;
    } else {
        return -1;
    }
}

/*
 * patricia_delete
 *
 * Delete a key from the patricia tree
 */
int
patricia_delete (patricia_tree_t *tree, char *key)
{
    /* Sanity check */
    if (!tree || !key) {
        return -1;
    }

    return patricia_delete_internal(tree, tree->root, key);
}

/*
 * patricia_add_internal
 *
 * This is a recursive routine which determines the place where the given key
 * is to be added and inserts it to the tree
 */
static int
patricia_add_internal (patricia_tree_t *tree, patricia_node_t *cur_node, 
                       char *key)
{
    int prefix_len, ret = 0;
    uint8_t insert_done;
    char *new_key, *common_key, *prev_key, *next_key;
    patricia_node_t *child, *next_child, *prev_node, *next_node, *new_node;

    /* Sanity check */
    if (!tree || !cur_node || !key) {
        return 0;
    }

    /* 
     * Get the length of the longest common prefix between the given key and
     * the key stored in the current node
     */
    prefix_len = patricia_get_prefix_count(key, cur_node->key);

    /*
     * We have 4 cases:
     *
     * 1. prefix_len = 0
     * 2. prefix_len < len(key), prefix_len >= len(cur_node->key)
     * 3. prefix_len = len(cur_node->key)
     * 4. cur_node = root
     *
     * For cases (1), (2) and (4), update the search key by removing the
     * common prefix and search for the node with this new key among the 
     * list of children recursively.
     *
     * For case (3), we need to split the current node into a root, left
     * and right node. The root node will contain the common prefix. The
     * left node will contain the suffix of the original key of current node.
     * The right node will contain the suffix of the new key to be inserted.
     * The left and right nodes will be determined based on lexicographical
     * ordering.
     */
    if (cur_node == tree->root || prefix_len == 0 ||
        (prefix_len > 0 && 
         prefix_len < strlen(key) && 
         prefix_len >= strlen(cur_node->key))) {
        /* Case 1, 2, 4 */
        new_key = substring(key, prefix_len, strlen(key) - prefix_len);
        if (!new_key) {
            return -1;
        }

        insert_done = 0;

        child = (patricia_node_t *)list_get_head(cur_node->children);
        while (child) {
            next_child = (patricia_node_t *)list_get_next(cur_node->children, child);
            if (child->key[0] == new_key[0]) {
                insert_done = 1;
                ret = patricia_add_internal(tree, child, new_key);
                if (ret != 0) {
                    insert_done = 0;
#ifdef PATRICIA_STATS_ON
                    stats.total_mem -= strlen(new_key);
#endif
                    free(new_key);
                    return -1;
                } else {
#ifdef PATRICIA_STATS_ON
                    stats.total_mem -= strlen(new_key);
#endif
                    free(new_key);
                    return 0;
                }
            }
            child = next_child;
        }

        if (insert_done == 0) {
            new_node = patricia_node_init(new_key, 1);
            patricia_add_child_node(cur_node, new_node);
        }

#ifdef PATRICIA_STATS_ON
        stats.total_mem -= strlen(new_key);
#endif
        free(new_key);

    } else if (prefix_len < strlen(key)) {
        /* Case 3 */
        common_key = substring(key, 0, prefix_len);
        prev_key = substring(cur_node->key, prefix_len, 
                             strlen(cur_node->key) - prefix_len);
        next_key = substring(key, prefix_len, strlen(key) - prefix_len);

        prev_node = patricia_node_init(prev_key, 0);
        prev_node->children = cur_node->children;

        next_node = patricia_node_init(next_key, 1);

        free(cur_node->key);
#ifdef PATRICIA_STATS_ON
        stats.total_mem -= strlen(cur_node->key);
#endif
        cur_node->key = common_key;
        cur_node->children = list_create();
#ifdef PATRICIA_STATS_ON
        stats.total_mem += sizeof(list_t);
#endif
        patricia_add_child_node(cur_node, prev_node);
        patricia_add_child_node(cur_node, next_node);

    } else if (prefix_len == strlen(key)) {
        /* 
         * This happens when we are asked to insert a key that is a prefix of
         * an existing key. In this case, we replace the existing key with
         * the prefix and create a new node for the remaining key.
         */
        common_key = substring(key, 0, prefix_len);
        next_key = substring(cur_node->key, prefix_len,
                             strlen(cur_node->key) - prefix_len);

        if (prefix_len == strlen(cur_node->key)) {
            /* Nothing to do */
            free(common_key);
            free(next_key);
            return 0;
        }

        next_node = patricia_node_init(next_key, 0);
        next_node->children = cur_node->children;

        free(cur_node->key);
#ifdef PATRICIA_STATS_ON
        stats.total_mem -= strlen(cur_node->key);
#endif
        cur_node->key = common_key;
        cur_node->children = list_create();
#ifdef PATRICIA_STATS_ON
        stats.total_mem += sizeof(list_t);
#endif
        patricia_add_child_node(cur_node, next_node);
    }

    return 0;
}

/*
 * patricia_add
 *
 * Add a new key to the patricia tree. Returns 0 upon success, -1 upon failure.
 */
int
patricia_add (patricia_tree_t *tree, char *key)
{
    /* Sanity check */
    if (!tree || !key) {
        return -1;
    }

    return patricia_add_internal(tree, tree->root, key);
}

/*
 * patricia_destroy
 *
 * Cleanup the given patricia tree instance
 */
int
patricia_destroy (patricia_tree_t *tree)
{
    free(tree->root->key);
    list_destroy(tree->root->children);
    free(tree->root);
    free(tree);
#ifdef PATRICIA_STATS_ON
    stats.total_mem += (sizeof(patricia_tree_t) + sizeof(patricia_node_t) + 
                        sizeof(list_t) + PATRICIA_ROOT_KEYLEN);
    stats.total_nodes--;
#endif

    return 0;
}

/*
 * patricia_init
 *
 * Initialize the patricia root and return a pointer back to the caller
 */
patricia_tree_t *
patricia_init (void)
{
    patricia_tree_t *tree;
    patricia_node_t *root;

    tree = (patricia_tree_t *)malloc(sizeof(patricia_tree_t));
    if (!tree) {
        return NULL;
    }

    root = (patricia_node_t *)malloc(sizeof(patricia_node_t));
    if (!root) {
        free(tree);
#ifdef PATRICIA_STATS_ON
        stats.total_mem -= sizeof(patricia_tree_t);
#endif
        return NULL;
    }

    root->key = (char *)malloc(PATRICIA_ROOT_KEYLEN);
    root->key[0] = 0;
    root->children = list_create();
    if (!root->children) {
        return NULL;
    }

#ifdef PATRICIA_STATS_ON
    stats.total_mem += (sizeof(patricia_tree_t) + sizeof(patricia_node_t) + 
                        sizeof(list_t) + PATRICIA_ROOT_KEYLEN);
    stats.total_nodes++;
#endif

    tree->root = root;
    return tree;
}

/* End of File */
