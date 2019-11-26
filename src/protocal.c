#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include "protocal.h"

HTTP_Req *http_req_parser(struct evbuffer *evb) {
    // read request line
    HTTP_Req *req = (HTTP_Req *)malloc(sizeof(HTTP_Req));
    memset(req, 0, sizeof(HTTP_Req));
    char *msg = evbuffer_readln(evb, NULL, EVBUFFER_EOL_CRLF);
    char type[10];
    sscanf(msg, "%s", type);
    if (!strcmp("GET", type)) {
        req->type = REQ_GET;
    }
    else if (!strcmp("POST", type)) {
        req->type = REQ_POST;
    }
    else {
        free(req);
        return NULL;
    }
    char *url = (char *)malloc(512 * sizeof(char));
    char *version = (char *)malloc(20 * sizeof(char));
    sscanf(msg, "%*s %s %s", url, version);
    req->url = url;
    req->version = version;

    // read header
    char *header = (char *)malloc(4096 * sizeof(char));
    memset(header, 0, 4096 * sizeof(char));

    while ((msg = evbuffer_readln(evb, NULL, EVBUFFER_EOL_CRLF)) && strlen(msg) > 0) {
        header = strcat(header, msg);
        header = strcat(header, "\r\n");
    }
    req->header = header;

    // read body
    char *body = (char *)malloc(4096 * sizeof(char));
    memset(body, 0, 4096 * sizeof(char));
    if (msg) {
        while ((msg = evbuffer_readln(evb, NULL, EVBUFFER_EOL_CRLF))) {
            body = strcat(body, msg);
            body = strcat(body, "\r\n");
        }
        req->body = body;
    }
    
    return req; 
}

HTTP_Ack *http_create_ack(HTTP_Req *req) {
    HTTP_Ack *ack = (HTTP_Ack *)malloc(sizeof(HTTP_Ack));
    memset(ack, 0, sizeof(HTTP_Ack));
    ack->version = "HTTP/1.1";
    ack->parser_code = 0;
    if (!req) {
        ack->parser_code |= HTTP_BAD_REQ;
        ack->status_code = HTTP_BADREQUEST;
        ack->state = "Bad Request";
        return ack;
    }
    ack->type = req->type;
    if (ack->type == REQ_GET /*&& !check(req->url)*/) {
        ack->status_code = HTTP_NOTFOUND;
        ack->state = "Not Found";
    }
    evhttp_decode_uri(req->url);
    evhttp_encode_uri(req->url);
    ack->header = strcat(ack->header, "Server: HTTPS-Server\r\n");
    ack->header = strcat(ack->header, "Date: \r\n");
    ack->header = strcat(ack->header, "Connection: keep-alive\r\n");
    ack->header = strcat(ack->header, "Content-Type: \r\n");
    ack->header = strcat(ack->header, "Content-Length: \r\n");
    ack->header = strcat(ack->header, "Transfer-Encoding: \r\n");

    ack->body = NULL;
    return ack;
}

