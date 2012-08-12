/*
 * patricia.h - Private header file for the patricia tree datastructure
 *
 * Dileep Ramesh, July 2012
 */

#ifndef PATRICIA_H
#define PATRICIA_H

#include <stdint.h>
#include "list.h"

/* Defines */

#define PATRICIA_ROOT_KEYLEN    1           /* Root node will store 0 as the key */
#define PATRICIA_DEFAULT_KEYLEN 256
#define PATRICIA_PREFIX_BUFSIZE 512000000   /* 1000000 keys each of length 512 bytes */

#define PATRICIA_STATS_ON		            /* For Debugging */

/* Datastructures */

typedef struct patricia_node_s {
    list_elem_t link;
    char        *key;
    list_t      *children;
} patricia_node_t;

typedef struct patricia_tree_s {
    patricia_node_t *root;
} patricia_tree_t;

#ifdef PATRICIA_STATS_ON
typedef struct patricia_stats_s {
    unsigned long   total_mem;
    unsigned long   total_keys;
    unsigned long   total_nodes;
} patricia_stats_t;
#endif

/* Function Prototypes */

void patricia_get_key_count (patricia_node_t *root, unsigned long *count);
void patricia_print_stats (patricia_tree_t *tree);
int patricia_lookup (patricia_tree_t *tree, char *key);
int patricia_lookup_prefix_partial (patricia_tree_t *tree, 
                                    char *prefix, char *buf);
int patricia_lookup_prefix_full (patricia_tree_t *tree, 
                                 char *prefix, char *buf);
int patricia_delete (patricia_tree_t *tree, char *key);
int patricia_add (patricia_tree_t *tree, char *key);
int patricia_destroy (patricia_tree_t *tree);
patricia_tree_t *patricia_init (void);

#endif /* PATRICIA_H */
