//  Copyright © 2017-2025

//  json.h
//  Support Json Decoder and Json Encoder.
//  Support Type JsonObject, that can use in array, number, dict.

//  Coder: zsy
//   Mail: admin@skysy.cn
//   Date: 2017.9.30

#ifndef SCLIB_JSON_JSON__H
#define SCLIB_JSON_JSON__H

#include <stddef.h>

#define MAX_JSON_DICT_SIZE              2048

#define JSON_TYPE_INTEGER               1001
#define JSON_TYPE_STRING                1002
#define JSON_TYPE_DECIMAL               1003
#define JSON_TYPE_DICT                  1004
#define JSON_TYPE_ARRAY                 1005

typedef struct JsonDictType{
    char                        c;
    
    struct JsonObjectStruct*    value;

    struct JsonDictType*        child;

    struct JsonDictType*        brother;
} JsonDict;

typedef union JsonObjectType {
    long long                   integer;

    char*                       string;

    double                      decimal;

    struct JsonDictType*        dict;

    struct JsonObjectStruct**   array;
} JsonObjectTp;

typedef struct JsonObjectStruct {
    int                         type;

    JsonObjectTp                object;
} JsonObject;

JsonObject*                 JsonDecoder(const char*, const unsigned int);

unsigned int                JsonEncoder(const JsonObject*, char*, const unsigned int);

int                         JsonDictSet(JsonObject*, const char*, const JsonObject*);

JsonObject*                 JsonDictGet(JsonObject*, const char*);

JsonObject*                 NewJsonDict();

JsonObject*                 NewJsonArray(const unsigned int);

JsonObject*                 NewJsonString(const char*);

JsonObject*                 NewJsonInteger(const long long);

JsonObject*                 NewJsonDecimal(const double);

void                        FreeJsonObject(JsonObject*);

#endif
