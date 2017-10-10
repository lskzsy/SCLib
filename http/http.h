//  Copyright © 2017-2025

//  http.h
//  Support http1.0 Get Post and Cookie.
//  Support sending async request in linux 2.6 and more hignly.
//  Support compiler has windows mingw, linux gcc, linux clang and os x clang.

//  Coder: zsy
//   Mail: admin@skysy.cn
//   Date: 2017.9.28

#ifndef SCLIB_HTTP_HTTP__H
#define SCLIB_HTTP_HTTP__H

#define MAX_HEADER_DATA_NUM             64
#define MAX_HTTP_BUFFER_SIZE            5120
#define MAX_HTTP_HEADER_BUFFER_SIZE     4096
#define MAX_POST_DATA_SIZE              4096
#define DEFAULT_RECV_CONTENT_SIZE       65535

#define HTTP_VERSION            "HTTP/1.0"
#define HTTP_GET                "GET"
#define HTTP_POST               "POST"
#define USER_AGENT              "SClib HttpTool/1.0 (compatible; Any OS)"

typedef struct {
    char*           name;
    
    char*           value;
    
    char*           expires;
    
    char*           path;
    
    char*           domain;
    
    unsigned short  secure;
} CookieNode;

typedef struct CookieType {
    char                chr;
    
    CookieNode*         v;
    
    struct CookieType*  child;

    struct CookieType*  brother;
} Cookie;

typedef struct {
    char*           protocal;
    
    char*           host;
    
    char*           address;
    
    unsigned short  port;    
    
    char*           uri;
} Url;

typedef struct HeaderKeyNode{
    char                    chr;

    short                   v;

    struct HeaderKeyNode*   child;

    struct HeaderKeyNode*   brother;
} HeaderKey;

typedef struct {
    HeaderKey*      keys;

    char*           values[MAX_HEADER_DATA_NUM];

    int             count;
} HttpHeader;

typedef HttpHeader HttpPostData;

typedef struct {
    //  http version
    char*           version;
    //  http status
    unsigned short  status;
    //  http status-message
    char*           message;
    //  http response header
    HttpHeader*     header;
    //  http response content
    char*           content;
} HttpResponse;

typedef struct {
    char*          method;

    Url*           url;

    HttpHeader*    header;

    char*          data;

    unsigned int   timeout;

    Cookie*        cookie;
} HttpRequest;

typedef void (*HttpCallback) (HttpResponse*);

//  Define interface function
HttpResponse*       HttpSend(const HttpRequest*);

short               AsyncHttpSend(const HttpRequest*, const HttpCallback);

HttpRequest*        NewHttpRequest(const char*, const char*, const HttpPostData*, Cookie*);

void                DeleteHttpRequest(HttpRequest*);

void                FreeHttpResponse(HttpResponse*);

void                SetHttpHeader(HttpHeader*, const char*, const char*);

void                SetHttpPostData(HttpPostData*, const char*, const char*);

HttpPostData*       CreateHttpPostData();

void                DestoryHttpPostData(HttpPostData*);

char*               GetHttpHeader(const HttpHeader*, const char*);

void                SetHttpTimeout(HttpRequest*, const unsigned int);

long long           StringToLong(const char*);

void                LongToString(const long long, char*, int);

int                 URLEncode(const char*, const int, char*, const int);

Cookie*             NewHttpCookie();

CookieNode*         NewHttpCookieNode(const char*, const char*, const char*, const char*, const char*);

void                SetHttpCookie(Cookie*, const char*, const CookieNode*);

void                GetHttpCookie(Cookie*, const char*, CookieNode*);

void                DeleteHttpCookie(Cookie*);

void                DeleteHttpCookieNode(CookieNode*);

#endif
