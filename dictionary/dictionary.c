#include "dictionary.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

TreeNode*   _generator_tree_node(char word, const void* data_ptr, size_t mem_size);
size_t      _find_tree_node(TreeNode** final, const char* key);
void        _free_tree_node_data(TreeNode* node);
void        _destory_tree_node(TreeNode* node);


Dictionary* create_dictionary()
{
    Dictionary* dict = (Dictionary*) calloc(1, sizeof(Dictionary));
    if (dict == NULL) {
        return dict;
    }

    dict->size = 0;
    dict->root = _generator_tree_node('\0', NULL, 0);
    return dict;
}

bool set_dictionary(Dictionary* dict, const char* key, const void* value, size_t mem_size)
{
    size_t     key_len  = strlen(key);
    size_t     pattern  = 0;
    TreeNode*  found    = dict->root;

    pattern = _find_tree_node(&found, key);
    for (size_t i = pattern; i < key_len; i++) {
        if (found->child == NULL) {
            if ((found->child = _generator_tree_node(key[i], NULL, 0)) == NULL) {
               return false; 
            }
            found = found->child;
        } else {
            found = found->child;
            while (found->brother != NULL) {
                found = found->brother;
            }

            if ((found->brother = _generator_tree_node(key[i], NULL, 0)) == NULL) {
               return false; 
            }
            found = found->brother;
        }
    }

    if (pattern != key_len) {
        dict->size++;
    }
    _free_tree_node_data(found);
    found->data_ptr = malloc(mem_size);
    memcpy(found->data_ptr, value, mem_size);
    return true;   
}

void* get_dictionary(Dictionary* dict, const char* key)
{
    size_t     key_len  = strlen(key);
    size_t     pattern  = 0;
    TreeNode*  found    = dict->root;

    if ((pattern = _find_tree_node(&found, key)) == key_len) {
        return found->data_ptr;
    } else {
        return NULL;
    }
}

bool remove_dictionary(Dictionary* dict, const char* key)
{
    size_t     key_len  = strlen(key);
    size_t     pattern  = 0;
    TreeNode*  found    = dict->root;

    if ((pattern = _find_tree_node(&found, key)) == key_len) {
        _free_tree_node_data(found);
        dict->size--;
        return true;
    } else {
        return false;
    }
}

void destory_dictionary(Dictionary* dict)
{
    _destory_tree_node(dict->root);
    free(dict);
}


TreeNode* _generator_tree_node(char word, const void* data_ptr, size_t mem_size)
{
    TreeNode* node = (TreeNode*) calloc(1, sizeof(TreeNode));
    if (node == NULL) {
        return node;
    }

    node->word = word;
    if (data_ptr != NULL) {
        node->data_ptr = malloc(mem_size);
        memcpy(node->data_ptr, data_ptr, mem_size);
    }
    return node;
}

size_t _find_tree_node(TreeNode** final, const char* key)
{
    if (*final == NULL || (*final)->child == NULL) {
        return 0;
    }

    size_t      key_len = strlen(key);
    TreeNode*   found   = NULL;
    for (size_t i = 0; i < key_len; i++) {
        found = (*final)->child;
        while (found != NULL) {
            if (found->word == key[i]) {
                break;
            }

            found = found->brother;
        }
        if (found == NULL) {
            return i;
        }

        *final = found;
    }

    return key_len;
}

void _free_tree_node_data(TreeNode* node)
{
    if (node != NULL && node->data_ptr != NULL) {
        free(node->data_ptr);
        node->data_ptr = NULL;
    }
}

void _destory_tree_node(TreeNode* node)
{
    if (node->brother != NULL) {
        _destory_tree_node(node->brother);
    }

    if (node->child != NULL) {
        _destory_tree_node(node->child);
    }

    if (node->data_ptr != NULL) {
        free(node->data_ptr);
    }
    free(node);
}
