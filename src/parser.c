#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <http_parser.h>
#include "parser.h"

char *HOME_DIR;
char *status_line;
char *header;
char *body;
char *req_url;
int status_code;

typedef enum {TYPE_NOTKNOWN, TYPE_DIR, TYPE_REG} filetype;
filetype ft;

void filepath_parse(char *path);
void notfound();
void forbidden();
void header_set_server();
void header_set_date();
void header_set_content_type();
void header_set_content_length();
void re_res_msg(http_parser *parser);

int on_message_begin(http_parser *parser) {
    header = malloc(1024);
    memset(header, 0, 1024);
    body = malloc(16 * 1024);
    memset(body, 0, 16 * 1024);
    status_code = 0;
    ft = TYPE_NOTKNOWN;
    req_url = NULL;
    return 0;
}

int on_url(http_parser *parser, const char *at, size_t length) {
    if (HTTP_GET == parser->method) {
        char *path = malloc(256);
        sprintf(path, "%s%.*s", HOME_DIR, (int)length, at);
        filepath_parse(path);
        re_res_msg(parser);
    }
    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t length) {
#ifdef DEBUG
    printf("Field: %.*s\n", (int)length, at);
#endif
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length) {
#ifdef DEBUG
    printf("Value: %.*s\n", (int)length, at);
#endif
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) {
    if (HTTP_POST == parser->method) {
        char *path = malloc(256);
        sprintf(path, "%s/%.*s", HOME_DIR, (int)length, at);
        filepath_parse(path);
        re_res_msg(parser);
    }
    return 0;
}

void re_res_msg(http_parser *parser) {
    header_set_server();
    header_set_date();
    header_set_content_type();
    header_set_content_length();
    
    parser->data = strcat(parser->data, status_line);
    parser->data = strcat(parser->data, header);
    parser->data = strcat(parser->data, "\r\n");
    parser->data = strcat(parser->data, body);
}

void header_set_server() {
    header = strcat(header, "Server: HTTP-Server/0.1\r\n");
}

void header_set_date() {
    char buf[1024];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof(buf), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
    header = strcat(header, buf);
}

void header_set_content_type() {
    char *mime = NULL;
    if (ft == TYPE_REG) {
        if (strstr(req_url, ".gif"))
            mime = "image/gif";
        else if (strstr(req_url, ".jpg"))
            mime = "image/jpeg";
        else if (strstr(req_url, ".png"))
            mime = "image/png";
        else if (strstr(req_url, ".txt"))
            mime = "text/plain";
        else if (strstr(req_url, ".pdf"))
            mime = "application/pdf";
        else if (strstr(req_url, ".exe"))
            mime = "application/x-msdownload";
        else
            mime = "application/octet-stream";
    }
    else 
        mime="text/html";

    char ct[128];
    sprintf(ct, "Content-Type: %s; charset=utf-8\r\n", mime);
    header = strcat(header, ct);
}

void header_set_content_length() {
    char content_len[128];
    sprintf(content_len, "Content-Length: %ld\r\n", strlen(body));
    header = strcat(header, content_len);
}

void filepath_parse(char *path) {
#ifdef DEBUG
    printf("URL: %s\n", path);
#endif
    req_url = path;
    struct stat st;
    if (stat(path, &st) < 0) {
        status_code = 404;
        status_line = "HTTP/1.1 404 NOT FOUND\r\n";
        notfound();
    }
    else if (!(st.st_mode & S_IROTH)) {
        status_code = 403;
        status_line = "HTTP/1.1 403 FORBIDDEN\r\n";
        forbidden();
    }
    else if (S_ISDIR(st.st_mode)) {
        ft = TYPE_DIR;
        status_code = 200;
        status_line = "HTTP/1.1 200 OK\r\n";
        char buf[1024];
        DIR *d = opendir(path);
        struct dirent *dir;
        body = strcat(body, "<html>\r\n");
        body = strcat(body, "<head>\r\n");
        body = strcat(body, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n");
        sprintf(buf, "<title>Directory listing for %s</title>\r\n", path+1);
        body = strcat(body, buf);
        body = strcat(body, "</head>\r\n");
        body = strcat(body, "<body>\r\n");
        sprintf(buf, "<h1>Directory listing for %s</h1>\r\n", path+1);
        body = strcat(body, buf);
        body = strcat(body, "<hr>\r\n");
        body = strcat(body, "<ul>\r\n");
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) {
                if (!strcmp(dir->d_name , ".") || !strcmp(dir->d_name, "..")) continue;
                sprintf(buf, "<li><a href=\"%s/\">%s/</a></li>\r\n", dir->d_name, dir->d_name);
                body = strcat(body, buf);
            }   
            else if (dir->d_type == DT_REG) {
                sprintf(buf, "<li><a href=\"%s\">%s</a></li>\r\n", dir->d_name, dir->d_name);
                body = strcat(body, buf);
            }    
        }
        closedir(d);
        body = strcat(body, "</ul>\r\n");
        body = strcat(body, "<hr>\r\n");
        body = strcat(body, "</body>\r\n");
        body = strcat(body, "</html>\r\n");
    }
    else {
        ft = TYPE_REG;
        status_code = 200;
        status_line = "HTTP/1.1 200 OK\r\n";

        FILE *fp = fopen(path, "r");
        int size = fread(body, 1, 16 * 1024, fp);
        fclose(fp);
    }
}

void notfound() {
    body = strcat(body, "<html>\r\n");
    body = strcat(body, "\t<head>\r\n");
    body = strcat(body, "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\r\n");
    body = strcat(body, "\t\t<title>Error response</title>\r\n");
    body = strcat(body, "\t</head>\r\n");
    body = strcat(body, "\t<body>\r\n");
    body = strcat(body, "\t\t<h1>Error response</h1>\r\n");
    body = strcat(body, "\t\t<p>Error code: 404</p>\r\n");
    body = strcat(body, "\t\t<p>Message: File not found.</p>\r\n");
    body = strcat(body, "\t\t<p>Error code explanation: HTTPStatus.NOT_FOUND - Nothing matches the given URI.</p>\r\n");
    body = strcat(body, "\t</body>\r\n");
    body = strcat(body, "</html>\r\n");
}

void forbidden() {
    body = strcat(body, "<html>\r\n");
    body = strcat(body, "\t<head>\r\n");
    body = strcat(body, "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\r\n");
    body = strcat(body, "\t\t<title>Error response</title>\r\n");
    body = strcat(body, "\t</head>\r\n");
    body = strcat(body, "\t<body>\r\n");
    body = strcat(body, "\t\t<h1>Error response</h1>\r\n");
    body = strcat(body, "\t\t<p>Error code: 403</p>\r\n");
    body = strcat(body, "\t\t<p>Message: Forbidden.</p>\r\n");
    body = strcat(body, "\t\t<p>Error code explanation: HTTPStatus.Forbidden.</p>\r\n");
    body = strcat(body, "\t</body>\r\n");
    body = strcat(body, "</html>\r\n");
}