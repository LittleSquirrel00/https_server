#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <http_parser.h>
#include "parser.h"

char *HOME_DIR;

int on_message_begin(http_parser *parser) {
    printf("\n***MESSAGE BEGIN***\n\n");
    return 0;
}

int on_url(http_parser *parser, const char *at, size_t length) {
    if (HTTP_GET == parser->method) {
        char path[256];
        sprintf(path, "%s%.*s", HOME_DIR, (int)length, at);
        DIR *d;
        FILE *fp;
        if ((d = opendir(path))) {
            // 目录文件
            struct dirent *dir;
            while ((dir = readdir(d)) != NULL) {
                printf("%s\n", dir->d_name);
            }
            closedir(d);
        }
        else if ((fp = fopen(path, "r"))){
            // 传输文件
            fclose(fp);
        }
        else {
            // 404
        }
    }
    return 0;
}

int on_status(http_parser *parser, const char *at, size_t length) {
    printf("Status: %.*s\n", (int)length, at);
    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t length) {
    printf("Header field: %.*s\n", (int)length, at);
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length) {
    printf("Header value: %.*s\n", (int)length, at);
    return 0;
}

int on_headers_complete(http_parser *parser) {
    printf("\n***HEADERS COMPLETE***\n\n");
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) {
    printf("Body: %.*s\n", (int)length, at);
    if (HTTP_POST == parser->method) {
        char path[256];
        sprintf(path, "%s/%.*s", HOME_DIR, (int)length, at);
        DIR *d;
        FILE *fp;
        if ((d = opendir(path))) {
            // 目录文件
            struct dirent *dir;
            while ((dir = readdir(d)) != NULL) {
                printf("%s\n", dir->d_name);
            }
            closedir(d);
        }
        else if ((fp = fopen(path, "r"))){
            // 传输文件
            fclose(fp);
        }
        else {
            // 404
        }
    }
    return 0;
}

int on_message_complete(http_parser *parser) {
    printf("\n***MESSAGE COMPLETE***\n\n");
    return 0;
}

int on_chunk_header(http_parser *parser) {
    printf("\n***CHUNK HEADER***\n\n");
    return 0;
}

int on_chunk_complete(http_parser *parser) {
    printf("\n***CHUNK COMPLETE***\n\n");
    return 0;
}