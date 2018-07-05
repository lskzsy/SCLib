#ifndef SCLIB_JSON_DECODER__H
#define SCLIB_JSON_DECODER__H

#include <unistd.h>
#include <stdarg.h>

typedef void (*param_callback) (const char*);

typedef void (*incomplete_callback) (const char* lack);

typedef enum {
    _int = 1,
    _string = 2,
    _boolean = 3,
    _function = 4,
    _volution = 5,
    _float = 6
} param_type;

void param_register(
    const char* op, 
    const void* variable, 
    size_t variable_size, 
    param_type variable_type,
    const void* _default);

int param_decode(int argc, const char* argv[]);

int param_complete(incomplete_callback callback, int count, ...);

#endif
