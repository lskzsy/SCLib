#include "json.h"

#include <stdio.h>

#include <string.h>
#include <stdlib.h>

int                 ScanSymbol(const char*, const unsigned int, int*);

JsonObject*         DecodeNumber(const char*, const unsigned int, const unsigned int);
JsonObject*         DecodeString(const char*, const unsigned int, const unsigned int);
JsonObject*         DecodeArray(const char*, const int*, const unsigned int, const unsigned int);
JsonObject*         DecodeDict(const char*, const int*, const unsigned int, const unsigned int);

JsonDict*           CreateJsonDict(const char);
JsonObject**        CreateJsonArray(const unsigned int);

void                DestoryJsonDict(JsonDict*);

void                VarDump(const JsonObject*, char*, unsigned int,unsigned int*);
void                DictDump(const JsonDict*, char*, unsigned int, char*, unsigned int,unsigned int*);

void DictDump(const JsonDict* dict, char* stack, unsigned int deep, char* buffer, unsigned int bufferLength,unsigned int* length)
{
    int wri     = 0;

    stack[deep++] = dict->c;
    if (dict->value != NULL) {
        wri = snprintf(buffer + *length, bufferLength - *length, "\"%s\":", stack + 1);
        *length += wri;
        VarDump(dict->value, buffer, bufferLength, length);
        wri = snprintf(buffer + *length, bufferLength - *length, ",");
        *length += wri;
    }

    if (dict->child != NULL) {
        DictDump(dict->child, stack, deep, buffer, bufferLength, length);
    }

    deep--;

    if (dict->brother != NULL) {
        DictDump(dict->brother, stack, deep, buffer, bufferLength, length);
    }
}

void VarDump(const JsonObject* object, char* buffer, unsigned int bufferLength,unsigned int* length)
{
    unsigned int iter = 0;
    unsigned int len  = 0; 
    unsigned int wri  = 0;
    char* stack = (char*) calloc(1000, sizeof(char));

    // printf("type:%d\n", object->type);
    switch(object->type) {
        case JSON_TYPE_ARRAY:
            wri = snprintf(buffer + *length, bufferLength - *length, "[");
            *length += wri;
            while (object->object.array[iter] != NULL) {
                VarDump(object->object.array[iter++], buffer, bufferLength, length);
                wri = snprintf(buffer + *length, bufferLength - *length, ",");
                *length += wri;
            }
            snprintf(buffer + *length - 1, bufferLength - *length, "]");
            break;
        case JSON_TYPE_INTEGER:
            wri = snprintf(buffer + *length, bufferLength - *length, "%lld", object->object.integer);
            *length += wri;
            break;
        case JSON_TYPE_DECIMAL:
            wri = snprintf(buffer + *length, bufferLength - *length, "%lf", object->object.decimal);
            *length += wri;
            while (buffer[*length - 1] == '0') {
                if (buffer[*length - 2] == '.') {
                    break;
                }

                *length -= 1;
            }
            break;
        case JSON_TYPE_STRING:
            snprintf(buffer + *length, bufferLength - *length, "\"");
            *length += 1;
            len = strlen(object->object.string);
            for (int i = 0; i < len; i++) {
                if (object->object.string[i] == '\\' || object->object.string[i] == '{' || object->object.string[i] == '[' ||
                    object->object.string[i] == '}' || object->object.string[i] == ']') {
                    snprintf(buffer + *length, bufferLength - *length, "\\");
                    *length += 1;
                }

                snprintf(buffer + *length, bufferLength - *length, "%c", object->object.string[i]);
                *length += 1;
            }
            snprintf(buffer + *length, bufferLength - *length, "\"");
            *length += 1;
            // wri = snprintf(buffer + *length, bufferLength - *length, "\"%s\"", object->object.string);
            // *length += wri;
            break;
        case JSON_TYPE_DICT:
            wri = snprintf(buffer + *length, bufferLength - *length, "{");
            *length += wri;
            DictDump(object->object.dict, stack, 0, buffer, bufferLength, length);
            snprintf(buffer + *length - 1, bufferLength - *length, "}");
            break;
    }

    free(stack);
}

unsigned int JsonEncoder(const JsonObject* object, char* buffer, const unsigned int bufferLength)
{
    unsigned int    length;

    VarDump(object, buffer, bufferLength, &length);

    return length;
}

JsonObject* NewJsonString(const char* string)
{
    JsonObject* object  = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->type        = JSON_TYPE_STRING;

    unsigned int length = strlen(string);
    object->object.string = (char*) calloc(length + 1, sizeof(char));
    strncpy(object->object.string, string, length);

    return object;
}

JsonObject* NewJsonInteger(const long long integer)
{
    JsonObject* object  = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->type        = JSON_TYPE_INTEGER;
    object->object.integer = integer;
    return object;
}

JsonObject* NewJsonDecimal(const double decimal)
{
    JsonObject* object  = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->type        = JSON_TYPE_DECIMAL;
    object->object.decimal = decimal;
    return object;
}


int JsonDictSet(JsonObject* object, const char* key, const JsonObject* value)
{
    if (object->type != JSON_TYPE_DICT) {
        return -1;
    }

    int needNew     = 0;
    int keyLen      = strlen(key);

    JsonDict* p = object->object.dict;
    for (int i = 0; i < keyLen; i++) {
        if (p->child == NULL) {
            p->child = CreateJsonDict(key[i]);
            needNew = 1;
        }
        p = p->child;
        while (p->c != key[i] && p->brother != NULL) {
            p = p->brother;
        }

        if (p->c != key[i] && p->brother == NULL) {
            p->brother = CreateJsonDict(key[i]);
            p = p->brother;
            needNew = 1;
        }
    }

    if (!needNew && p->value != NULL) {
        FreeJsonObject(p->value);
    }
    p->value = (struct JsonObjectStruct *)value;

    return 0;
}

JsonObject* JsonDictGet(JsonObject* object, const char* key)
{
    if (object->type != JSON_TYPE_DICT) {
        return NULL;
    }

    int keyLen      = strlen(key);

    JsonDict* p = object->object.dict;
    for (int i = 0; i < keyLen; i++) {
        if (p->child == NULL) {
            return NULL;
        }
        p = p->child;
        while (p->c != key[i] && p->brother != NULL) {
            p = p->brother;
        }

        if (p->c != key[i] && p->brother == NULL) {
            return NULL;
        }
    }

    return p->value;
}

void FreeJsonObject(JsonObject* object)
{
    unsigned int iter = 0;

    switch (object->type) {
        case JSON_TYPE_DICT:
            DestoryJsonDict(object->object.dict);
            break;
        case JSON_TYPE_STRING:
            free(object->object.string);
            break;
        case JSON_TYPE_ARRAY:
            while (object->object.array[iter] != NULL) {
                FreeJsonObject(object->object.array[iter++]);
            }
            break;
    }

    free(object);
}

void DestoryJsonDict(JsonDict* dict)
{
    if (dict->child != NULL) {
        DestoryJsonDict(dict->child);
    }

    if (dict->brother != NULL) {
        DestoryJsonDict(dict->brother);
    }

    if (dict->value != NULL) {
        FreeJsonObject(dict->value);
    }

    free(dict);
}

JsonObject* NewJsonDict()
{
    JsonObject* object  = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->type        = JSON_TYPE_DICT;
    object->object.dict = CreateJsonDict('\0');
    return object;
}

JsonObject* NewJsonArray(const unsigned int size)
{
    JsonObject* object      = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->type            = JSON_TYPE_ARRAY;
    object->object.array    = CreateJsonArray(size);
    return object;
}

JsonDict* CreateJsonDict(const char c)
{
    JsonDict* dict = (JsonDict*) calloc(1, sizeof(JsonDict));
    dict->c = c;
    return dict;
}

JsonObject** CreateJsonArray(const unsigned int size)
{
    return (JsonObject**) calloc(size + 1, sizeof(JsonObject*));
}

//  {"a":"bbbb","b":[1,2,3,4,5],"c":{"a1":"b2","a2":"\""}}
int ScanSymbol(const char* input, const unsigned int length, int* symbolMap)
{
    unsigned int  deep  = 0;
    int           word  = 0;
    char          temp  = '\0';
    unsigned int* stack = (unsigned int*) calloc(length, sizeof(int));

    for (unsigned int i = 0; i < length; i++) {
        if (i != 0 && input[i - 1] == '\\') {
            continue;
        }
        
        temp = input[i];
        if ((temp == '"' && !word) || temp == '{' || temp == '[') {
            stack[deep++] = i;

            if (temp == '"') {
                word = !word;
            }
            continue;
        }

        if ((temp == '"' && word) || temp == '}' || temp == ']') {
            symbolMap[i]                = stack[deep - 1];
            symbolMap[stack[deep - 1]]  = i;
            deep--;

            if (temp == '"') {
                word = !word;
            }
        }
    }

    if (deep != 0) {
        return -1;
    }

    free(stack);
    return 0;
}

JsonObject* DecodeNumber(const char* strJson, const unsigned int left, const unsigned int right)
{
    JsonObject* object      = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->type            = JSON_TYPE_INTEGER;

    long long   integer     = 0;
    double      decimal     = 0.1;
    int         negative    = 0;
    unsigned    begin       = left;

    if (strJson[begin] == '-') {
        negative = 1;
        begin++;
    }

    for (unsigned int i = begin; i <= right; i++) {
        if (strJson[i] != '.' && (strJson[i] < '0' || strJson[i] > '9')) {
            FreeJsonObject(object);
            return NULL;    
        }

        if (strJson[i] == '.') {
            if (object->type == JSON_TYPE_DECIMAL) {
                FreeJsonObject(object);
                return NULL; 
            }

            object->type = JSON_TYPE_DECIMAL;
            object->object.decimal += integer;
            continue;
        }

        if (object->type == JSON_TYPE_INTEGER) {
            integer *= 10;
            integer += (strJson[i] - '0');
        } else {
            object->object.decimal += (strJson[i] - '0') * decimal;
            decimal *= 0.1;
        }
    }

    if (object->type == JSON_TYPE_INTEGER) {
        object->object.integer = integer;
    }

    if (negative) {
        if (object->type == JSON_TYPE_INTEGER) {
            object->object.integer *= -1;
        } else {
            object->object.decimal *= -1;
        }
    }

    return object;
}

JsonObject* DecodeString(const char* strJson, const unsigned int left, const unsigned int right)
{
    JsonObject* object      = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->type            = JSON_TYPE_STRING;

    unsigned int length     = right - left + 1;
    unsigned int strIter    = 0;
    object->object.string   = (char*) calloc(length, sizeof(char));

    for (unsigned int i = left; i <= right; i++) {
        if (strJson[i] == '\\') {
            i++;
        }

        object->object.string[strIter++] = strJson[i];
    }

    object->object.string[strIter] = '\0';
    return object;
}

JsonObject* DecodeArray(const char* strJson, const int* symbolMap, const unsigned int left, const unsigned int right)
{
    JsonObject* object      = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->type            = JSON_TYPE_ARRAY;

    unsigned int arraySize  = 1;
    unsigned int arrayIter  = 0;
    int          doubt      = 0;
    unsigned int doubtLeft  = 0;

    for (unsigned int i = left; i <= right; i++) {
        if (strJson[i - 1] != '\\' && (strJson[i] == '{' || strJson[i] == '[')) {
            i = symbolMap[i];
            continue;
        }

        if (strJson[i] == ',' && strJson[i - 1] != '\\') {
            arraySize++;
        }
    }

    object->object.array = CreateJsonArray(arraySize);
    // for (int i = 0; i <= arraySize; i++) {
    //     printf("%x ", object->object.array[i]);
    // }
    // printf("\n");
    for (unsigned int i = left; i <= right; i++) {
        if (strJson[i - 1] == '\\') {
            continue;
        }

        switch (strJson[i]) {
            case ' ':
                break;
            case '{':
                object->object.array[arrayIter++] = DecodeDict(strJson, symbolMap, i + 1, symbolMap[i] - 1);
                if (object->object.array[arrayIter - 1] == NULL) {
                    FreeJsonObject(object);
                    return NULL;
                }
                i = symbolMap[i];
                break;
            case '[':
                object->object.array[arrayIter++] = DecodeArray(strJson, symbolMap, i + 1, symbolMap[i] - 1);
                if (object->object.array[arrayIter - 1] == NULL) {
                    FreeJsonObject(object);
                    return NULL;
                }
                i = symbolMap[i];
                break;
            case '"':
                object->object.array[arrayIter++] = DecodeString(strJson, i + 1, symbolMap[i] - 1);
                if (object->object.array[arrayIter - 1] == NULL) {
                    FreeJsonObject(object);
                    return NULL;
                }
                i = symbolMap[i];
                break;
            case ',':
                if (doubt) {
                    object->object.array[arrayIter++] = DecodeNumber(strJson, doubtLeft, i - 1);
                    if (object->object.array[arrayIter - 1] == NULL) {
                        FreeJsonObject(object);
                        return NULL;
                    }
                    doubt = 0;
                }
                break;
            default:
                if (!doubt) {
                    doubt = 1;
                    doubtLeft = i;
                }
                break;
        }
    }
    if (doubt) {
        object->object.array[arrayIter++] = DecodeNumber(strJson, doubtLeft, right);
        if (object->object.array[arrayIter - 1] == NULL) {
            FreeJsonObject(object);
            return NULL;
        }
        doubt = 0;
    }

    // object->object.array[arrayIter] = NULL;
    // for (int i = 0; i <= arrayIter; i++) {
    //     printf("%x ", object->object.array[i]);
    // }
    // printf("\n");

    return object;
}

JsonObject* DecodeDict(const char* strJson, const int* symbolMap, const unsigned int left, const unsigned int right)
{
    JsonObject* object      = (JsonObject*) calloc(1, sizeof(JsonObject));
    object->object.dict     = CreateJsonDict('\0'); 
    object->type            = JSON_TYPE_DICT;

    int          isValue                     = 0;
    unsigned int keyLen                      = 0;
    char         keyBuf[MAX_JSON_DICT_SIZE]  = {0};
    int          doubt                       = 0;
    unsigned int doubtLeft                   = 0;
    JsonObject*  value                       = NULL;

    for (unsigned int i = left; i <= right; i++) {
        // printf("%c\n", strJson[i]);
        if (strJson[i] == '"' && !isValue) {
            keyLen = symbolMap[i] - i - 1;
            // printf("%d %d\n", symbolMap[i], i);
            strncpy(keyBuf, &strJson[i + 1], keyLen);
            keyBuf[keyLen] = '\0';
            i = symbolMap[i];
            continue;
        }

        if (strJson[i] == ':' && strJson[i - 1] != '\\') {
            isValue = 1;
            continue;
        }

        if (strJson[i] == ',' && strJson[i - 1] != '\\') {
            if (doubt) {
                value = DecodeNumber(strJson, doubtLeft, i - 1);
                if (value == NULL) {
                    FreeJsonObject(object);
                    return NULL;
                }
                JsonDictSet(object, keyBuf, value);
                doubt = 0;
            }

            isValue = 0;
            continue;
        }

        if (isValue && !doubt) {
            switch (strJson[i]) {
                case ' ':
                    break;
                case '{':
                    // printf("decode dict\n");
                    value = DecodeDict(strJson, symbolMap, i + 1, symbolMap[i] - 1);
                    if (value == NULL) {
                        FreeJsonObject(object);
                        return NULL;
                    }
                    JsonDictSet(object, keyBuf, value);
                    i = symbolMap[i];
                    break;
                case '[':
                    // printf("decode array\n");
                    value = DecodeArray(strJson, symbolMap, i + 1, symbolMap[i] - 1);
                    if (value == NULL) {
                        FreeJsonObject(object);
                        return NULL;
                    }
                    JsonDictSet(object, keyBuf, value);
                    i = symbolMap[i];
                    break;
                case '"':
                    // printf("decode string\n");
                    value = DecodeString(strJson, i + 1, symbolMap[i] - 1);
                    if (value == NULL) {
                        FreeJsonObject(object);
                        return NULL;
                    }
                    JsonDictSet(object, keyBuf, value);
                    i = symbolMap[i];
                    break;
                default:
                    doubtLeft = i;
                    doubt = 1;
                    break;
            }
        }
    }

    if (doubt) {
        value = DecodeNumber(strJson, doubtLeft, right);
        if (value == NULL) {
            FreeJsonObject(object);
            return NULL;
        }
        JsonDictSet(object, keyBuf, value);
        doubt = 0;
    }

    return object;
}

JsonObject* JsonDecoder(const char* strJson, const unsigned int length)
{
    int*            symbolMap = (int*) calloc(length, sizeof(int));
    JsonObject*     object    = NULL;

    int ret = ScanSymbol(strJson, length, symbolMap);
    if (ret < 0) {
        free(symbolMap);
        return NULL;
    }
    // for (int i = 0; i < length; i++) {
    //     printf("%d ", symbolMap[i]);
    // }
    // printf("\n");

    switch (strJson[0]) {
        case '{':
            object = DecodeDict(strJson, symbolMap, 1, length - 2);
            break;
        case '[':
            object = DecodeArray(strJson, symbolMap, 1, length - 2);
            break;
        default:
            return NULL;
    }

    free(symbolMap);
    return object;
}

//  demo.c
// void dict_dump(const JsonDict*, char*, unsigned int, const int);
// void var_dump(const JsonObject*, const int);

// void printTab(const int tab) 
// {
//     int count = tab * 4;
//     for (int i = 0; i < count; i++) {
//         printf(" ");
//     }
// }

// void dict_dump(const JsonDict* dict, char* stack, unsigned int deep, const int tab)
// {
//     stack[deep++] = dict->c;
//     if (dict->value != NULL) {
//         printTab(tab);
//         printf("\"%s\":\n", stack + 1);
//         var_dump(dict->value, tab + 1);
//         printf(",\n");
//     }

//     if (dict->child != NULL) {
//         dict_dump(dict->child, stack, deep, tab);
//     }

//     deep--;

//     if (dict->brother != NULL) {
//         dict_dump(dict->brother, stack, deep, tab);
//     }
// }

// void var_dump(const JsonObject* object, const int tab)
// {
//     int   iter  = 0;
//     char* stack = (char*) calloc(1000, sizeof(char));

//     // printf("type:%d\n", object->type);
//     switch(object->type) {
//         case JSON_TYPE_ARRAY:
//             printTab(tab);
//             printf("[\n");
//             while (object->object.array[iter] != NULL) {
//                 var_dump(object->object.array[iter++], tab + 1);
//                 printf(",\n");
//             }
//             printTab(tab);
//             printf("]");
//             break;
//         case JSON_TYPE_INTEGER:
//             printTab(tab);
//             printf("%lld", object->object.integer);
//             break;
//         case JSON_TYPE_DECIMAL:
//             printTab(tab);
//             printf("%lf", object->object.decimal);
//             break;
//         case JSON_TYPE_STRING:
//             printTab(tab);
//             printf("\"%s\"", object->object.string);
//             break;
//         case JSON_TYPE_DICT:
//             printTab(tab);
//             printf("{\n");
//             dict_dump(object->object.dict, stack, 0, tab + 1);
//             printTab(tab);
//             printf("}");
//             break;
//     }

//     free(stack);
// }

// //  [{"a":[1,"猝死从根好"],"b":6},"2",["c","d"],0.4,100,-2.3,{"a":-2.3}]
// int main(void)
// {
//     char str[256];
//     char out[256];
//     printf("Input Json: ");
//     scanf("%s", str);

//     JsonObject* json = JsonDecoder(str, strlen(str));
//     if (json == NULL) {
//         printf("Json Decode error.\n");
//         return 0;
//     }
//     printf("var_dump(json): ");
//     var_dump(json, 0);
//     printf("\n");
//     printf("================================\n");
//     printf("JsonDictGet(json->object.array[0], \"a\")->object.array[1]->object.string => %s\n", JsonDictGet(json->object.array[0], "a")->object.array[1]->object.string);
//     printf("================================\n");
//     int len = JsonEncoder(json, out, 256);
//     printf("%d: %s\n", len, out);
//     FreeJsonObject(json);
//     return 0;
// }
