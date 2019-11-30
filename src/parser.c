#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
// #include <unistd.h>
#include <dirent.h>
#include <http_parser.h>
#include "parser.h"

char *HOME_DIR;
HTTP_Response *res;

void filepath_parse(char *path);

void notfound();

void forbidden();

void header_set_server();

void header_set_date();

void header_set_content_type();

void header_set_content_length();

int on_message_begin(http_parser *parser) {
    res = malloc(sizeof(res));
    memset(res, 0, sizeof(res));
    res->header = malloc(1024);
    res->body = malloc(1024);
    memset(res->header, 0, 1024);
    memset(res->body, 0, 1024);
    return 0;
}

int on_url(http_parser *parser, const char *at, size_t length) {
    if (HTTP_GET == parser->method) {
        char *path = malloc(256);
        sprintf(path, "%s%.*s", HOME_DIR, (int)length, at);
        filepath_parse(path);
    }
    return 0;
}

int on_status(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int on_headers_complete(http_parser *parser) {
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) {
    if (HTTP_POST == parser->method) {
        char *path = malloc(256);
        sprintf(path, "%s/%.*s", HOME_DIR, (int)length, at);
        filepath_parse(path);
    }
    return 0;
}

int on_message_complete(http_parser *parser) {
    header_set_server();
    header_set_date();
    header_set_content_type();
    header_set_content_length();
    
    parser->data = strcat(parser->data, res->status_line);
    parser->data = strcat(parser->data, res->header);
    parser->data = strcat(parser->data, "\r\n");
    parser->data = strcat(parser->data, res->body);
    
    // free(res->header);
    // free(res->body);
    free(res);
    return 0;
}

void header_set_server() {
    res->header = strcat(res->header, "Server: HTTP-Server/0.1\r\n");
}

void header_set_date() {
    char buf[1024];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof(buf), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
    res->header = strcat(res->header, buf);
}

void header_set_content_type() {
    char *mime = NULL;
    if (res->status_code == 200) {
        if (S_ISDIR(res->file_stat->st_mode))
            mime="text/html";
        else if (strstr(res->req_url, ".gif"))
            mime="image/gif";
        else if (strstr(res->req_url, ".jpg"))
            mime="image/jpeg";
        else if (strstr(res->req_url, ".png"))
            mime="image/png";
        else
            mime="application/octet-stream";
    }
    else 
        mime="text/html";
    
    char ct[128];
    sprintf(ct, "Content-Type: %s; charset=utf-8\r\n", mime);
    res->header = strcat(res->header, ct);
}

void header_set_content_length() {
    char content_len[128];
    sprintf(content_len, "Content-Length: %ld\r\n", strlen(res->body));
    res->header = strcat(res->header, content_len);
}

int on_chunk_header(http_parser *parser) {
    return 0;
}

int on_chunk_complete(http_parser *parser) {
    return 0;
}

void filepath_parse(char *path) {
    res->req_url = path;
    res->file_stat = malloc(sizeof(struct stat));
    if (stat(path, res->file_stat) < 0) {
        res->status_code = 404;
        res->status_line = "HTTP/1.1 404 NOT FOUND\r\n";
        notfound();
    }
    else if (!(res->file_stat->st_mode & S_IROTH)) {
        res->status_code = 403;
        res->status_line = "HTTP/1.1 403 FORBIDDEN\r\n";
        forbidden();
    }
    else if (S_ISDIR(res->file_stat->st_mode)) {
        res->status_code = 200;
        res->status_line = "HTTP/1.1 200 OK\r\n";
        char buf[1024];
        DIR *d = opendir(path);
        struct dirent *dir;
        res->body = strcat(res->body, "<html>\r\n");
        res->body = strcat(res->body, "<head>\r\n");
        res->body = strcat(res->body, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n");
        sprintf(buf, "<title>Directory listing for %s</title>\r\n", path);
        res->body = strcat(res->body, buf);
        res->body = strcat(res->body, "</head>\r\n");
        res->body = strcat(res->body, "<body>\r\n");
        res->body = strcat(res->body, "<h1>Directory listing for /</h1>\r\n");
        res->body = strcat(res->body, "<hr>\r\n");
        res->body = strcat(res->body, "<ul>\r\n");
        while ((dir = readdir(d)) != NULL) {
            sprintf(buf, "<li><a href=\"%s\">%s</a></li>\r\n", dir->d_name, dir->d_name);
            res->body = strcat(res->body, buf);
        }
        closedir(d);
        res->body = strcat(res->body, "</ul>\r\n");
        res->body = strcat(res->body, "<hr>\r\n");
        res->body = strcat(res->body, "</body>\r\n");
        res->body = strcat(res->body, "</html>\r\n");
    }
    else {
        res->status_code = 200;
        res->status_line = "HTTP/1.1 200 OK\r\n";
        // FILE *fp = fopen(path, "r");
        // fread(fp, );
        // int fp = open(path, "r");
        // int size = read(fp, );
        // close(fp);
    }
}

void notfound() {
    res->body = strcat(res->body, "<html>\r\n");
    res->body = strcat(res->body, "\t<head>\r\n");
    res->body = strcat(res->body, "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\r\n");
    res->body = strcat(res->body, "\t\t<title>Error response</title>\r\n");
    res->body = strcat(res->body, "\t</head>\r\n");
    res->body = strcat(res->body, "\t<body>\r\n");
    res->body = strcat(res->body, "\t\t<h1>Error response</h1>\r\n");
    res->body = strcat(res->body, "\t\t<p>Error code: 404</p>\r\n");
    res->body = strcat(res->body, "\t\t<p>Message: File not found.</p>\r\n");
    res->body = strcat(res->body, "\t\t<p>Error code explanation: HTTPStatus.NOT_FOUND - Nothing matches the given URI.</p>\r\n");
    res->body = strcat(res->body, "\t</body>\r\n");
    res->body = strcat(res->body, "</html>\r\n");
}

void forbidden() {
    res->body = strcat(res->body, "<html>\r\n");
    res->body = strcat(res->body, "\t<head>\r\n");
    res->body = strcat(res->body, "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\r\n");
    res->body = strcat(res->body, "\t\t<title>Error response</title>\r\n");
    res->body = strcat(res->body, "\t</head>\r\n");
    res->body = strcat(res->body, "\t<body>\r\n");
    res->body = strcat(res->body, "\t\t<h1>Error response</h1>\r\n");
    res->body = strcat(res->body, "\t\t<p>Error code: 403</p>\r\n");
    res->body = strcat(res->body, "\t\t<p>Message: Forbidden.</p>\r\n");
    res->body = strcat(res->body, "\t\t<p>Error code explanation: HTTPStatus.Forbidden.</p>\r\n");
    res->body = strcat(res->body, "\t</body>\r\n");
    res->body = strcat(res->body, "</html>\r\n");
}