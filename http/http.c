#include "http.h"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

int                 IsDomain(const char*);
void                AddNewLine(char*);
Url*                GetUrlFromString(const char*);
void                FreeUrl(Url*);

HttpHeader*         CreateHeader();
HeaderKey*          CreateHeaderKeyNode(char);
void                DestoryHeader(HttpHeader*);
void                DestoryHeaderKeyNode(HeaderKey*);
unsigned int        MakeRequestPacket(const HttpRequest*, char*);

void                ErgodicHeaderKeyTree(const HttpHeader* ,const HeaderKey*, char*, int, char*, int*, int);
unsigned int        GetPostDataBuffer(const HttpHeader*, char*);
unsigned int        GetHeaderBuffer(const HttpHeader*, char*);
unsigned int        GetResponseHead(const char*, int, HttpResponse*);
void                AppendReponseContent(HttpResponse*, const char*, const unsigned int);
void                SetResponseContentBufferSize(HttpResponse*, long long);

int                 InitConnection(const char*, const unsigned short, const unsigned int);

int IsDomain(const char* input)
{
    int dot = 0;
    int len = strlen(input);
    for (int i = 0; i < len; i++) {
        if ((input[i] < '0' || input[i] > '9') && input[i] != '.') {
            return 1;
        }

        if (input[i] == '.') {
            dot++;
        }
    }

    return dot != 3;
}

HeaderKey* CreateHeaderKeyNode(char c)
{
    HeaderKey* key = (HeaderKey*) calloc(1, sizeof(HeaderKey));
    key->v = -1;
    key->chr = c;
    key->child = NULL;
    key->brother = NULL;
    return key;
}

void DestoryHeaderKeyNode(HeaderKey* key)
{
    if (key->child != NULL) {
        DestoryHeaderKeyNode(key->child);  
    }
    if (key->brother != NULL) {
        DestoryHeaderKeyNode(key->brother);
    }
    free(key);   
}

void DestoryHeader(HttpHeader* header)
{
    for (int i = 0; i < header->count; i++) {
        free(header->values[i]);
    }
    DestoryHeaderKeyNode(header->keys);
    free(header);
}

void FreeUrl(Url* url)
{
    if (url->host != url->address) {
        free(url->host);
    }
    free(url->protocal);
    free(url->address);
    free(url->uri);
}

HttpHeader* CreateHeader()
{
    HttpHeader* header;
    header = (HttpHeader*) calloc(1, sizeof(HttpHeader));
    header->keys = CreateHeaderKeyNode('*');

    return header;
}

void ErgodicHeaderKeyTree(const HttpHeader* header, const HeaderKey* key, char* stack, int head, char* buffer, int* length, int op)
{
    stack[head++] = key->chr;

    if (key->v != -1) {
        //  Leaf node
        if (op == 0) {
            int valueLen = strlen(header->values[key->v]);
            int len      = *length;
            //printf("%s %d %d\n", header->values[key->v], valueLen, len);

            char* p;
            p = buffer + len;
            strncpy(p, stack + 1, head - 1);
            p += head - 1;
            strncpy(p, ": ", 2);
            p += 2;
            strncpy(p, header->values[key->v], valueLen);
            p += valueLen;
            strncpy(p, "\r\n", 2);
            *length = len + head + valueLen + 3;
        } else if (op == 1) {
            // printf("%s\n", stack);
            int valueLen = strlen(header->values[key->v]);
            int len      = *length;
            // printf("%s %d %d\n", header->values[key->v], valueLen, len);

            char* p;
            p = buffer + len;
            strncpy(p, stack + 1, head - 1);
            p += head - 1;
            strncpy(p, "=", 1);
            p += 1;
            strncpy(p, header->values[key->v], valueLen);
            p += valueLen;
            strncpy(p, "&", 1);
            *length = len + head + valueLen + 1;
        }
    }
    if (key->child != NULL) {
        ErgodicHeaderKeyTree(header, key->child, stack, head, buffer, length, op);
    } 

    head--;

    if (key->brother != NULL) {
        ErgodicHeaderKeyTree(header, key->brother, stack, head, buffer, length, op);  
    }  
}

unsigned int GetPostDataBuffer(const HttpPostData* data, char* buffer)
{
    int             head = 0;
    char*           stack = (char*) calloc(1024, sizeof(char));
    int             bufferLength = 0;
    
    ErgodicHeaderKeyTree(data, data->keys, stack, head, buffer, &bufferLength, 1);
    buffer[bufferLength - 1] = '\0';
    // printf("%s %d\n", buffer, bufferLength);

    free(stack);
    return bufferLength;
}

unsigned int GetHeaderBuffer(const HttpHeader* header, char* buffer)
{
    int             head = 0;
    char*           stack = (char*) calloc(1024, sizeof(char));
    int             bufferLength = 0;
    
    ErgodicHeaderKeyTree(header, header->keys, stack, head, buffer, &bufferLength, 0);
    buffer[bufferLength] = '\0';

    free(stack);
    return bufferLength;
}

void AppendReponseContent(HttpResponse* response, const char* content, const unsigned int len)
{
    int size = strlen(response->content);
    strncpy(response->content + size, content, len);
}

void SetResponseContentBufferSize(HttpResponse* response, long long size)
{
    char* swap = (char*) calloc(size, sizeof(char));
    strncpy(swap, response->content, strlen(response->content));
    free(response->content);
    response->content = swap;
}

unsigned int GetResponseHead(const char* buffer, int bufferLength, HttpResponse* response)
{
    int setup   = 0;
    int i       = 0;
    int prev    = 0;
    int size    = 0;
    int split   = 0;

    char* key      = (char*) calloc(512, sizeof(char));
    char* value    = (char*) calloc(512, sizeof(char));
    char  strStatus[3];
    // printf("%s\n", buffer);
    while (i < bufferLength) {
        if (buffer[i] == '\r' || buffer[i] == ' ') {
            size = i - prev;     
            switch (setup) {
                case 0:
                    //  Get Http Version
                    response->version = (char*) calloc(size + 1, sizeof(char));
                    strncpy(response->version, buffer + prev, size);
                    response->version[size] = '\0';
                    break;
                case 1:
                    //  Get Http Status
                    strncpy(strStatus, buffer + prev, size);
                    response->status = (unsigned short)StringToLong(strStatus);
                    break;
                case 2:
                    //  Get Http Message
                    response->message = (char*) calloc(size + 1, sizeof(char));
                    strncpy(response->message, buffer + prev, size);
                    response->message[size] = '\0';
                    break;
                default:
                    //  Get Http Header
                    if (buffer[i] == ' ') {
                        i++;
                        continue;
                    }

                    // printf("%d %d %d\n", prev, split, i);
                    size = split - prev;
                    // printf("%d\n", size);
                    strncpy(key, buffer + prev, size);
                    key[size] = '\0';

                    size = i - split - 2;
                    // printf("%d\n", size);
                    strncpy(value, buffer + split + 2, size);
                    value[size] = '\0';
                    // printf("%s %s\n", key, value);
                    SetHttpHeader(response->header, key, value);
                    break;
            }

            if (buffer[i] == ' ') {
                i += 1;
            } else {
                i += 2;

                if (buffer[i] == '\r') {
                    i += 2;
                    break;
                }
            }

            prev = i;
            setup++;
            continue;
        }

        if (buffer[i] == ':') {
            split = i;
        }

        i++;
    }

    //  Save Content
    size = bufferLength - i;
    response->content = (char*) calloc(size + 1, sizeof(char));
    strncpy(response->content, buffer + i, size);
    // printf("%s\n", buffer + i);
    response->content[size] = '\0';

    free(key);
    free(value);
    return i;
}

int InitConnection(const char* destAddr, const unsigned short port, const unsigned int timeout)
{
    int                     sockfd;
    struct sockaddr_in      addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    addr.sin_family         = AF_INET;
    addr.sin_port           = htons(port);
    addr.sin_addr.s_addr    = inet_addr(destAddr);

    if (timeout != 0) {
        struct timeval T = {timeout, 0};
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&T, sizeof(T));
    }    

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1) {
        return -2;
    }

    return sockfd;
}

void AddNewLine(char* buffer)
{
    *buffer         = '\r';
    *(buffer + 1)   = '\n';
    *(buffer + 2)   = '\0';
}

Url* GetUrlFromString(const char* url)
{
    Url* answer = (Url*) calloc(1, sizeof(Url));
    int  urlLen = strlen(url);
    int  split  = 0;
    int  split2 = 0;

    int  protoR = -1;
    int  hostL  = 0;
    int  hostR  = -1;
    int  portL  = -1;
    int  uriL   = -1;

    for (int i = 0; i < urlLen; i++) {
        hostR++;
        if (url[i] == '/') {
            if (split == 0 && url[i + 1] == '/') {
                hostL = i + 2;
                i++;
            } else {
                hostR = i - 1;
                uriL = i;
                break;
            }

            split++;
        }

        if (url[i] == ':') {
            if (url[i + 1] == '/') {
                protoR = i - 1;
            }

            if (url[i + 1] >= '0' && url[i + 1] <= '9') {
                portL = i + 1;
            }

            split2++;
        }
    }

    //  protocal
    if (protoR != -1) {
        answer->protocal = (char*) calloc(protoR + 2, sizeof(char));
        strncpy(answer->protocal, url, protoR + 1);
        answer->protocal[protoR + 2] = '\0';
    } else {
        answer->protocal = (char*) calloc(5, sizeof(char));
        strncpy(answer->protocal, "http", 4);
    }

    //  host
    answer->host = (char*) calloc(hostR - hostL + 2, sizeof(char));
    strncpy(answer->host, url + hostL, hostR - hostL + 1);

    //  port
    if (portL == -1) {
        answer->port = 80;
    } else {
        char strPort[5] = {0};
        strncpy(strPort, url + portL, hostR - portL + 1);
        long long t = StringToLong(strPort);
        if (t < 0) {
            return NULL;
        }
        answer->port = (unsigned short) t;
    }

    //  address
    if (portL == -1) {
        answer->address = (char*) calloc(hostR - hostL + 2, sizeof(char));
        strncpy(answer->address, url + hostL, hostR - hostL + 1);
        answer->address[hostR - hostL + 2] = '\0';
    } else {
        answer->address = (char*) calloc(portL - hostL, sizeof(char));
        strncpy(answer->address, url + hostL, portL - hostL - 1);
        answer->address[portL - hostL] = '\0';
    }

    if (IsDomain(answer->address)) {
        struct hostent* addr = gethostbyname(answer->address);
        if (addr == NULL) {
            return NULL;
        }
        free(answer->address);
        answer->address = (char*) calloc(15, sizeof(char));
        inet_ntop(addr->h_addrtype, addr->h_addr, answer->address, 15);
    }

    if (uriL != -1) {
        answer->uri = (char*) calloc(urlLen - uriL + 1, sizeof(char));
        strncpy(answer->uri, url + uriL, urlLen - uriL);
    } else {
        answer->uri = (char*) calloc(1, sizeof(char));
        answer->uri[0] = '/';
    }

    return answer;
}

unsigned int MakeRequestPacket(const HttpRequest* request, char* buffer)
{
    unsigned int    bufferLength = 0;
    unsigned int    size = 0;
    char*           headerBuffer = (char* ) calloc(MAX_HTTP_HEADER_BUFFER_SIZE, sizeof(char));
    char            dataLength[5] = {0};

    if (request->data != NULL) {
        LongToString(strlen(request->data), dataLength, 5);
        SetHttpHeader(request->header, "Content-Length", dataLength);
    }

    //      Add http method
    size = strlen(request->method); 
    strncpy(buffer + bufferLength, request->method, size);
    buffer[bufferLength + size] = ' ';
    bufferLength += size + 1;

    //      Add http uri
    size = strlen(request->url->uri);
    strncpy(buffer + bufferLength, request->url->uri, size);
    buffer[bufferLength + size] = ' ';
    bufferLength += size + 1;

    //      Add http version
    size = strlen(HTTP_VERSION);
    strncpy(buffer + bufferLength, HTTP_VERSION, size);
    AddNewLine(buffer + bufferLength + size);
    bufferLength += size + 2;

    //      Add http header
    size = GetHeaderBuffer(request->header, headerBuffer);
    strncpy(buffer + bufferLength, headerBuffer, size);
    AddNewLine(buffer + bufferLength + size);
    bufferLength += size + 2;

    //      Add http data
    if (request->data != NULL) {
        size = strlen(request->data);
        strncpy(buffer + bufferLength, request->data, size);
        bufferLength += size;
    }
    buffer[bufferLength] = '\0';
    
    free(headerBuffer);
    return bufferLength;
}

HttpResponse* HttpSend(const HttpRequest* request)
{
    char*  buffer;
    char*  strContentLength;
    int    bufferLength;
    int    sockfd;
    int    result;
    int    headLength;
    int    sendLength;
    int    contentLength;
    int    contentNext;
    char*  recvBuffer;

    HttpResponse*   response = NULL;

    sockfd = InitConnection(request->url->address, request->url->port, request->timeout);
    if (sockfd < 1) {
        return NULL;
    }

    response = (HttpResponse*) calloc(1, sizeof(HttpResponse));
    response->header = CreateHeader();
    recvBuffer = (char* ) calloc(MAX_HTTP_BUFFER_SIZE, sizeof(char));
    buffer = (char* ) calloc(MAX_HTTP_BUFFER_SIZE, sizeof(char));
    
    bufferLength = MakeRequestPacket(request, buffer);
    sendLength = bufferLength;
    result = 0;
    do {
        result = send(sockfd, buffer + result, sendLength, 0);
        if (result <= 0) {
            return NULL;
        }

        sendLength -= result;
    } while (sendLength);

    result = recv(sockfd, recvBuffer, MAX_HTTP_BUFFER_SIZE, 0);
    if (result <= 0) {
        return NULL;
    }

    headLength       = GetResponseHead(recvBuffer, result, response);
    strContentLength = GetHttpHeader(response->header, "Content-Length");
    if (strContentLength == NULL) {
        contentLength = DEFAULT_RECV_CONTENT_SIZE;
    } else {
        contentLength = StringToLong(strContentLength);
    }
    
    // printf("%d %d %d\n", result, headLength, contentLength);
    if (result - headLength < contentLength) {
        //printf("111\n");
        SetResponseContentBufferSize(response, contentLength);
        contentNext = contentLength + headLength - result;
        struct timeval T = {1, 0};
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&T, sizeof(T));

        do {
            result = recv(sockfd, recvBuffer, MAX_HTTP_BUFFER_SIZE, 0);
            if (result <= 0) {
                break;
            }

            AppendReponseContent(response, recvBuffer, result);
            contentNext -= result;
        } while (contentNext);
    }

    close(sockfd);
    free(buffer);
    free(recvBuffer);

    return response;
}

long long StringToLong(const char* string)
{
    long long   answer = 0;
    int         length = strlen(string);

    for (int i = 0; i < length; i++) {
        if (string[i] < '0' || string[i] > '9') {
            return -1;
        }

        answer *= 10;
        answer += (string[i] - '0');
    }

    return answer;
}

void LongToString(const long long num, char* string, int len)
{
    long long   step = 1;
    int         i    = 0;
    while (1) {
        if (num % step == num) {
            break;
        }

        step *= 10;
        i++;
    }

    if (len < i) {
        return;
    }

    long long t = num;
    for (int j = i - 1; j >= 0; j--) {
        string[j] = (t % 10) + '0';
        t /= 10;
    }

    string[i] = '\0';
}

HttpRequest* NewHttpRequest(const char* url, const char* method, const HttpPostData* data)
{
    HttpRequest*    request;
    int             size;

    request = (HttpRequest*) calloc(1, sizeof(HttpRequest));
    request->data = NULL;
    
    request->url = GetUrlFromString(url);
    if (request->url == NULL) {
        return NULL;
    }

    size = strlen(method);
    request->method = (char*) calloc(size + 1, sizeof(char));
    strncpy(request->method, method, size);
    request->method[size] = '\0';

    request->header = CreateHeader();
    //  Default Header key-value
    SetHttpHeader(request->header, "Host", request->url->host);
    SetHttpHeader(request->header, "User-Agent", USER_AGENT);
    if (strcmp(request->method, "POST") == 0 && data != NULL) {
        SetHttpHeader(request->header, "Content-Type", "application/x-www-form-urlencoded");
        request->data = (char*) calloc(MAX_POST_DATA_SIZE, sizeof(char));
        GetPostDataBuffer(data, request->data);
    }

    return request;
}

void DeleteHttpRequest(HttpRequest* request)
{
    free(request->data);
    free(request->method);
    
    FreeUrl(request->url);
    DestoryHeader(request->header);

    free(request);
}

void FreeHttpResponse(HttpResponse* response)
{
    free(response->version);
    free(response->message);
    free(response->content);
    DestoryHeader(response->header);
    free(response);
}

void SetHttpPostData(HttpPostData* data, const char* key, const char* value)
{
    char buffer[1024] = {0};
    URLEncode(value, strlen(value), buffer, 1024);
    SetHttpHeader(data, key, buffer);
}

void SetHttpHeader(HttpHeader* header, const char* key, const char* value)
{
    int needNew     = 0;
    int keyLen      = strlen(key);
    int valueLen    = strlen(value);

    HeaderKey* p = header->keys;
    for (int i = 0; i < keyLen; i++) {
        if (p->child == NULL) {
            p->child = CreateHeaderKeyNode(key[i]);
            needNew = 1;
        }
        p = p->child;
        while (p->chr != key[i] && p->brother != NULL) {
            p = p->brother;
        }

        if (p->chr != key[i] && p->brother == NULL) {
            p->brother = CreateHeaderKeyNode(key[i]);
            p = p->brother;
            needNew = 1;
        }
    }

    if (needNew || p->v == -1) {
        header->values[header->count] = (char*) calloc(valueLen + 1, sizeof(char));
        strncpy(header->values[header->count], value, valueLen);
        p->v = header->count++;
    } else {
        free(header->values[p->v]);
        header->values[p->v] = (char*) calloc(valueLen + 1, sizeof(char));
        strncpy(header->values[p->v], value, valueLen);
    }
}

char* GetHttpHeader(const HttpHeader* header, const char* key)
{
    int keyLen      = strlen(key);

    HeaderKey* p = header->keys;
    for (int i = 0; i < keyLen; i++) {
        if (p->child == NULL) {
            return NULL;
        }
        p = p->child;
        while (p->chr != key[i] && p->brother != NULL) {
            p = p->brother;
        }

        if (p->chr != key[i] && p->brother == NULL) {
            return NULL;
        }
    }

    return header->values[p->v];
}

int URLEncode(const char* src, const int srcSize, char* dest, const int destSize)  
{  
    int i;  
    int j = 0;
    char ch;  
  
    if ((src == NULL) || (dest == NULL) || (srcSize <= 0) || (destSize <= 0)) {  
        return 0;  
    }  
  
    for (i=0; (i < srcSize) && (j < destSize); i++) {  
        ch = src[i];  
        if (((ch >= 'A') && (ch <= 'Z')) ||  
            ((ch >= 'a') && (ch <= 'z')) ||  
            ((ch >= '0') && (ch <= '9')) ||
            ch == '+') {  
            dest[j++] = ch;  
        } else if (ch == ' ') {  
            dest[j++] = '+';  
        } else if (ch == '.' || ch == '-' || ch == '_' || ch == '*') {  
            dest[j++] = ch;  
        } else {  
            if (j + 3 < destSize) {  
                sprintf(dest + j, "%%%02X", (unsigned char)ch);  
                j += 3;  
            } else {  
                return 0;  
            }  
        }  
    }  
  
    dest[j] = '\0';  
    return j;  
}

HttpPostData* CreateHttpPostData()
{
    return (HttpPostData*) CreateHeader();
}

void DestoryHttpPostData(HttpPostData* data)
{
    DestoryHeader((HttpHeader*)data);
}

void SetTimeout(HttpRequest* request, const unsigned int s)
{
    request->timeout = s;
}

short AsyncHttpSend(const HttpRequest* request, const HttpCallback callback)
{
    pid_t fpid = fork();
    if (fpid < 0) {
        return -1;
    }

    if (fpid == 0) {
        HttpResponse* response = HttpSend(request);
        callback(response);
        FreeHttpResponse(response);
    }

    return 0;
}

// demo.c
// void Callback(HttpResponse* response)
// {
//     //   Print Async Recv
//     {
//         printf("\nAsync Response\n");
//         printf("=====================================\n");
//         printf("Version: %s\n", response->version);
//         printf("Status: %d\n", response->status);
//         printf("Message: %s\n", response->message);
//         printf("Content:\n");
//         printf("%s\n", response->content);
//         printf("=====================================\n");
//     }
// }

// int main(void) {
//     //  test
//     char str[10240];
//     int  method;
//     printf("Url:");
//     scanf("%s", str);
//     printf("Method:");
//     scanf("%d", &method);

//     HttpRequest* request;
//     if (method == 0) {
//         request = NewHttpRequest(str, HTTP_GET, NULL);
//         SetTimeout(request, 3);
//     } else {
//         HttpPostData* data = CreateHttpPostData();
//         SetHttpPostData(data, "country", "SE");
//         SetHttpPostData(data, "search", "port:80+start:1+len:1000");
//         SetHttpPostData(data, "ipsearch", "查询");
//         SetHttpPostData(data, "ip", "");
//         SetHttpPostData(data, "port", "");

//         request = NewHttpRequest(str, HTTP_POST, data);
//         SetTimeout(request, 3);
//         SetHttpHeader(request->header, "Cookie", "JSESSIONID=66E7C827DC215EA19F0CAB4E1905D9F5; JSESSIONID=8E0A5625B0576899A4D6DF9E2B567DF9");
//         DestoryHttpPostData(data);
//     }

//     //   Print Send
//     {
//         printf("URL\n");
//         printf("=====================================\n");
//         if (request->url == NULL) {
//             printf("Error!\n");
//         } else {
//             printf("Protocal: %s\n", request->url->protocal);
//             printf("Host: %s\n", request->url->host);
//             printf("Uri: %s\n", request->url->uri);
//             printf("Port: %d\n", request->url->port);
//             printf("Address: %s\n", request->url->address);
//         }
//         printf("=====================================\n");

//         MakeRequestPacket(request, str);
//         printf("\n");
//         printf("Request\n");
//         printf("=====================================\n");
//         for (int i = 0; ; i++) {
//             if (str[i] == '\0') {
//                 break;
//             }

//             printf("%02x ", (short)str[i]);
//         }
//         printf("\n\n%s", str);
//         printf("\n=====================================\n");
//     }

//     HttpResponse* response = HttpSend(request);
//     if (response == NULL) {
//         printf("Http error.\n");
//         DeleteHttpRequest(request);
//         return 0;
//     }

//     //   Print Recv
//     {
//         printf("\nResponse\n");
//         printf("=====================================\n");
//         printf("Version: %s\n", response->version);
//         printf("Status: %d\n", response->status);
//         printf("Message: %s\n", response->message);
//         printf("Content:\n");
//         printf("%s\n", response->content);
//         printf("=====================================\n");
//     }

//     AsyncHttpSend(request, Callback);

//     FreeHttpResponse(response);
//     DeleteHttpRequest(request);
//     return 0;
// }
