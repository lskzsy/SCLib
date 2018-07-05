#include "decoder.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/*
 *  Date structure
 */
typedef struct {
    const void* variable;

    size_t variable_size;

    param_type variable_type;

    const void* _default;

    bool found;
} param_register_info;

typedef struct param_register_info_tree_node {
    char                                    chr;
    
    param_register_info*                    v;
    
    struct param_register_info_tree_node*   child;

    struct param_register_info_tree_node*   brother;
} param_register_info_set;


/*
 *  Global variables
 */
param_register_info_set param_register_info_tree_root   = {0};
int                     param_index                     = 0;
int                     param_num                       = 0;
const char**            param_array;


/*
 *  Function Declaration
 */
void                        param_decode_int(const char* string, const void* variable);
void                        param_decode_string(const char* string, const void* variable, size_t variable_size);
void                        param_decode_float(const char* string, const void* variable);
param_register_info*        create_param_register_info();
param_register_info_set*    create_param_register_info_tree_node(char c);
param_register_info*        get_param_register_info(const char* op);
void                        set_param_register_info_tree_node(const char* op, param_register_info* info);
void                        init_param(int argc, const char* argv[]);
const char*                 get_next_param();

/*
 *  Function
 */
void 
init_param(int argc, const char* argv[])
{
    param_num   = argc;
    param_array = argv;
}

const char*                 
get_next_param()
{
    if (param_index >= param_num) {
        return NULL;
    }

    return param_array[param_index++];
}

param_register_info* 
create_param_register_info()
{
    return (param_register_info*) calloc(1, sizeof(param_register_info));
}

param_register_info_set* 
create_param_register_info_tree_node(char c)
{
    param_register_info_set* node = (param_register_info_set*) calloc(1, sizeof(param_register_info_set));
    node->v = NULL;
    node->chr = c;
    node->child = NULL;
    node->brother = NULL;
    return node;
}

param_register_info*
get_param_register_info(const char* op)
{
    int len = strlen(op);

    param_register_info_set* p = &param_register_info_tree_root;
    for (int i = 0; i < len; i++) {
        if (p->child == NULL) {
            return NULL;
        }
        p = p->child;
        while (p->chr != op[i] && p->brother != NULL) {
            p = p->brother;
        }

        if (p->chr != op[i] && p->brother == NULL) {
            return NULL;
        }
    }

    return p->v;
}

void
set_param_register_info_tree_node(
    const char* op,
    param_register_info* info)
{
    int len = strlen(op);
    int needNew = 0;

    param_register_info_set* p = &param_register_info_tree_root;
    for (int i = 0; i < len; i++) {
        if (p->child == NULL) {
            p->child = create_param_register_info_tree_node(op[i]);
            needNew = 1;
        }
        p = p->child;
        while (p->chr != op[i] && p->brother != NULL) {
            p = p->brother;
        }

        if (p->chr != op[i] && p->brother == NULL) {
            p->brother = create_param_register_info_tree_node(op[i]);
            p = p->brother;
            needNew = 1;
        }
    }

    if (needNew || p->v == NULL) {
        p->v = info;
    } else {
        free(p->v);
        p->v = info;
    }
}

void
param_decode_int(
    const char* string,
    const void* variable)
{
    int* int_variable = (int*) variable;
    int minus = 1;
    int i = 0;
    int len = strlen(string);

    if (string[i] == '-') {
        i++;
        minus = -1;
    }

    for (; i < len; i++) {
        if (string[i] >= '0' && string[i] <= '9') {
            (*int_variable) *= 10;
            (*int_variable) += (string[i] - '0');
        }
    }
    
    (*int_variable) *= minus;
}

void
param_decode_float(
    const char* string,
    const void* variable)
{
    float* float_variable = (float*) variable;
    int minus = 1;
    int i = 0;
    int len = strlen(string);

    if (string[i] == '-') {
        i++;
        minus = -1;
    }

    for (; i < len; i++) {
        if (string[i] == '.') {
            break;
        } else if (string[i] >= '0' && string[i] <= '9') {
            (*float_variable) *= 10;
            (*float_variable) += (string[i] - '0');
        }
    }

    float float_content = 0;
    for (int j = len - 1; j > i; j--) {
        float_content *= 0.1;
        float_content += (string[j] - '0') * 0.1;
    }
    
    (*float_variable) += float_content;
    (*float_variable) *= minus;
}

void
param_decode_string(
    const char* string,
    const void* variable,
    size_t variable_size)
{
    char* string_variable = (char*) variable;
    strncpy(string_variable, string, variable_size);
}

/*
 *  Interface
 */
void 
param_register(
    const char* op, 
    const void* variable, 
    size_t variable_size, 
    param_type variable_type,
    const void* _default)
{
    param_register_info* info = create_param_register_info();
    info->variable = variable;
    info->variable_size = variable_size;
    info->variable_type = variable_type;
    info->_default = _default;

    set_param_register_info_tree_node(op, info);
}

int 
param_decode(
    int argc, 
    const char* argv[])
{
    init_param(argc, argv);

    size_t avaliable_param_count = 0;

    const char*             option;       
    param_register_info*    info;
    param_callback          callback;
    while ((option = get_next_param()) != NULL) {
        info = get_param_register_info(option);
        if (info == NULL) {
            continue;
        }

        switch (info->variable_type) {
            case _int:
                param_decode_int(get_next_param(), info->variable);
                info->found = true;
                avaliable_param_count++;
                break;
            case _string:
                param_decode_string(get_next_param(), info->variable, info->variable_size);
                info->found = true;
                avaliable_param_count++;
                break;
            case _function:
                callback = (param_callback) info->variable;
                callback(option);
                info->found = true;
                avaliable_param_count++;
                break;
            case _volution:
                memcpy((void*)info->variable, info->_default, info->variable_size);
                info->found = true;
                avaliable_param_count++;
                break;
            case _float:
                param_decode_float(get_next_param(), info->variable);
                info->found = true;
                avaliable_param_count++;
                break;
            default:
                break;
        }
    } 

    return avaliable_param_count;
}

int param_complete(incomplete_callback callback, int count, ...)
{
    va_list                 list;
    char*                   options;
    char                    option[256];
    int                     or_head = 0;
    int                     options_len = 0;
    bool                    found;
    param_register_info*    info;

    va_start(list, count);
    for (int i = 0; i < count; i++) {
        options = va_arg(list, char*);
        options_len = strlen(options);
        found = false;
        or_head = 0;
        for (int j = 0; j <= options_len; j++) {
            if (j == options_len || options[j] == '|') {
                strncpy(option, options + or_head, j - or_head);
                option[j - or_head] = '\0';
                info = get_param_register_info(option);
                if (info->found) {
                    found = true;
                }
                or_head = j + 1;
            }
        }

        if (!found) {
            callback(options);
            return 0;
        }
    }
    va_end(list);
    return 1;
}

//  demo.c
// void help(const char* op)
// {
//     printf("Usage: ...\n");
//     exit(0);
// }

// void lack(const char* op) {
//     printf("Lack param %s\n", op);
//     help(NULL);
// }

// int main(int argc, const char* argv[])
// {
//     int     type = 0;
//     int     tcp = 1;
//     int     udp = 2;
//     char    file[255] = {0};
//     int     limit = 0;
//     float   percent = 0;

//     param_register("-T", &type, sizeof(int), _volution, &tcp);
//     param_register("-U", &type, sizeof(int), _volution, &udp);
//     param_register("-f", file, 255, _string, NULL);
//     param_register("-l", &limit, sizeof(int), _int, NULL);
//     param_register("-p", &percent, sizeof(float), _float, NULL);
//     param_register("-h", help, 0, _function, NULL);
//     param_decode(argc, argv);
//     param_complete(lack, 3, "-T|-U", "-f", "-l");

//     printf("Type: %d\n", type);
//     printf("File: %s\n", file);
//     printf("Limit: %d\n", limit);
//     printf("Percent: %f\n", percent);
//     return 0;
// }
