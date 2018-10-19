#ifndef SCLIB_DICTIONARY__H
#define SCLIB_DICTIONARY__H

#include <unistd.h>
#include <stdbool.h>


typedef struct T_TreeNode {
    char                word;
    void*               data_ptr;
    struct T_TreeNode*  child;
    struct T_TreeNode*  brother;  
} TreeNode;

typedef struct {
    TreeNode*   root;
    size_t      size;
} Dictionary;


Dictionary*         create_dictionary();
bool                set_dictionary(Dictionary* dict, const char* key, const void* value, size_t mem_size);
void*               get_dictionary(Dictionary* dict, const char* key);
bool                remove_dictionary(Dictionary* dict, const char* key);
void                destory_dictionary(Dictionary* dict);

#endif
