#include <stdio.h>
#include <http_parser.h>
#include "parser.h"

int on_message_begin(http_parser *parser) {
    printf("\n***MESSAGE BEGIN***\n\n");
    return 0;
}

int on_url(http_parser *parser, const char *at, size_t length) {
    printf("Url: %.*s\n", (int)length, at);
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