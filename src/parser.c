#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <http_parser.h>
#include "parser.h"

void filepath_parse(http_parser *parser);
void notfound(http_parser *parser);
void forbidden(http_parser *parser);
void mkheader(Ack_Data *data);

int on_message_begin(http_parser *parser) {
    Ack_Data *data = parser->data;
    data->load = GET;
    return 0;
}

int on_url(http_parser *parser, const char *at, size_t length) { 
    Ack_Data *data = (Ack_Data *)parser->data;
    data->url = (char *)malloc(256);
    data->path = (char *)malloc(256);
    memset(data->url, 0, 256);
    memset(data->path, 0, 256);
    if (HTTP_GET == parser->method) {
        sprintf(data->url, "%.*s", (int)length, at);
        if (data->url_argv = strchr(data->url, '?')) {
            sprintf(data->path, "%s%.*s", data->home, 
                (int)(data->url_argv - data->url), data->url);
            data->url_argv++;
        }
        else {
            sprintf(data->path, "%s%.*s", data->home, (int)length, at);
        }
        filepath_parse(parser);
        mkheader(data);
    }
    else if (HTTP_POST == parser->method) {
        sprintf(data->path, "%s%.*s", data->home, (int)length, at);
    }
    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t length) {
    Ack_Data *data = (Ack_Data *)parser->data;
    char tmp[128];
    sprintf(tmp, "%.*s", (int)length, at);
    if (!data->content_type && strstr(tmp, "Content-Type")) {
        data->content_type = UNSET;
    }
    if (!data->content_length && strstr(tmp, "Content-Length")) {
        data->content_length = UNSET;
    }
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (data->content_type == UNSET) {
        data->content_type = (char *)malloc(256);
        sprintf(data->content_type, "%.*s", (int)length, at);
        char *begin;
        if (begin = strstr(data->content_type, "boundary=")) {
            begin += 9;
            data->boundary = (char *)malloc(128);
            sprintf(data->boundary, "%.*s", (int)(at + length - begin), begin);
        }
    }
    else if (data->content_length == UNSET) {
        data->content_length = (char *)malloc(32);
        sprintf(data->content_length, "%.*s", (int)length, at);
        if (!atoi(data->content_length)) {
            filepath_parse(parser);
            mkheader(data);
        }
    }
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (!data->boundary) {
        filepath_parse(parser);
        mkheader(data);
    }
    else {
        data->load = UPLOAD;
        FILE *fp = fopen(data->path, "a");
        int len = strlen(data->boundary);
        char *begin = strstr(at, data->boundary) + 1;
        char *end = strstr(begin, data->boundary) + 1;
        while (end) {
            for (int i = 0; i < 4; i++)
                begin = strchr(begin, '\n') + 1;
            int size = fwrite(begin, 1, (int)(end - 5 -  begin), fp);
            end = strstr(end, data->boundary);
        }
        fclose(fp);

        data->status_code = 200;
        data->status_line = "HTTP/1.1 200 OK\r\n";
        mkheader(data);
    }
    return 0;
}

void mkheader(Ack_Data *data) {
    data->header = (char *)malloc(1024);
    data->header = strcat(data->header, "Server: HTTP-Server/0.1\r\n");

    char buf[1024];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof(buf), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
    data->header = strcat(data->header, buf);
    
    char *mime = NULL;
    if (data->load == GET && S_ISREG(data->st->st_mode)) {
        if (strstr(data->path, ".gif"))
            mime = "image/gif";
        else if (strstr(data->path, ".jpg"))
            mime = "image/jpeg";
        else if (strstr(data->path, ".png"))
            mime = "image/png";
        else if (strstr(data->path, ".txt"))
            mime = "text/plain";
        else if (strstr(data->path, ".pdf"))
            mime = "application/pdf";
        else if (strstr(data->path, ".exe"))
            mime = "application/x-msdownload";
        else
            mime = "application/octet-stream";
    }
    else 
        mime="text/html";

    char ct[128];
    sprintf(ct, "Content-Type: %s; charset=utf-8\r\n", mime);
    data->header = strcat(data->header, ct);

    if (data->load == CHUNK) 
        data->header = strcat(data->header, "Transfer-Encoding: chunked\r\n");    
    char content_length[128];
    if (!data->body)
        sprintf(content_length, "Content-Length: 0\r\n");
    else 
        sprintf(content_length, "Content-Length: %ld\r\n", strlen(data->body));
    data->header = strcat(data->header, content_length);
}

void filepath_parse(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;

    data->st = (struct stat *)malloc(sizeof(struct stat));
    if (stat(data->path, data->st) < 0) {
        data->status_code = 404;
        data->status_line = "HTTP/1.1 404 NOT FOUND\r\n";
        notfound(parser);
    }
    else if (!(data->st->st_mode & S_IROTH)) {
        data->status_code = 403;
        data->status_line = "HTTP/1.1 403 FORBIDDEN\r\n";
        forbidden(parser);
    }
    else if (S_ISDIR(data->st->st_mode)) {
        data->status_code = 200;
        data->status_line = "HTTP/1.1 200 OK\r\n";
        char buf[1024];
        DIR *d = opendir(data->path);
        struct dirent *dir;
        data->body = (char *)malloc(80 *1024);
        data->body = strcat(data->body, "<html>\r\n");
        data->body = strcat(data->body, "<head>\r\n");
        data->body = strcat(data->body, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n");
        sprintf(buf, "<title>Directory listing for %s</title>\r\n", data->path+6);
        data->body = strcat(data->body, buf);
        data->body = strcat(data->body, "</head>\r\n");
        data->body = strcat(data->body, "<body>\r\n");
        sprintf(buf, "<h1>Directory listing for %s</h1>\r\n", data->path+6);
        data->body = strcat(data->body, buf);
        data->body = strcat(data->body, "<hr>\r\n");
        data->body = strcat(data->body, "<ul>\r\n");
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) {
                if (!strcmp(dir->d_name , ".") || !strcmp(dir->d_name, "..")) continue;
                sprintf(buf, "<li><a href=\"%s/\">%s/</a></li>\r\n", dir->d_name, dir->d_name);
                data->body = strcat(data->body, buf);
            }   
            else if (dir->d_type == DT_REG) {
                sprintf(buf, "<li><a href=\"%s\">%s</a></li>\r\n", dir->d_name, dir->d_name);
                data->body = strcat(data->body, buf);
            }    
        }
        closedir(d);
        data->body = strcat(data->body, "</ul>\r\n");
        data->body = strcat(data->body, "<hr>\r\n");
        data->body = strcat(data->body, "</body>\r\n");
        data->body = strcat(data->body, "</html>\r\n");
    }
    else {
        data->status_code = 200;
        data->status_line = "HTTP/1.1 200 OK\r\n";
        data->body = (char *)malloc(80 *1024);
        
        if (0) {
            data->load = CHUNK;
        }
        else if (data->st->st_size > 80 * 1024) {
            data->load = DOWNLOAD;
        }
        else {
            FILE *fp = fopen(data->path, "r");
            int size = fread(data->body, 1, 16 * 1024, fp);
            fclose(fp);
        }
    }
}

void notfound(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    data->body = (char *)malloc(1024);
    data->body = strcat(data->body, "<html>\r\n");
    data->body = strcat(data->body, "\t<head>\r\n");
    data->body = strcat(data->body, "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\r\n");
    data->body = strcat(data->body, "\t\t<title>Error response</title>\r\n");
    data->body = strcat(data->body, "\t</head>\r\n");
    data->body = strcat(data->body, "\t<body>\r\n");
    data->body = strcat(data->body, "\t\t<h1>Error response</h1>\r\n");
    data->body = strcat(data->body, "\t\t<p>Error code: 404</p>\r\n");
    data->body = strcat(data->body, "\t\t<p>Message: File not found.</p>\r\n");
    data->body = strcat(data->body, "\t\t<p>Error code explanation: HTTPStatus.NOT_FOUND - Nothing matches the given URI.</p>\r\n");
    data->body = strcat(data->body, "\t</body>\r\n");
    data->body = strcat(data->body, "</html>\r\n");
}

void forbidden(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    data->body = (char *)malloc(1024);
    data->body = strcat(data->body, "<html>\r\n");
    data->body = strcat(data->body, "\t<head>\r\n");
    data->body = strcat(data->body, "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\r\n");
    data->body = strcat(data->body, "\t\t<title>Error response</title>\r\n");
    data->body = strcat(data->body, "\t</head>\r\n");
    data->body = strcat(data->body, "\t<body>\r\n");
    data->body = strcat(data->body, "\t\t<h1>Error response</h1>\r\n");
    data->body = strcat(data->body, "\t\t<p>Error code: 403</p>\r\n");
    data->body = strcat(data->body, "\t\t<p>Message: Forbidden.</p>\r\n");
    data->body = strcat(data->body, "\t\t<p>Error code explanation: HTTPStatus.Forbidden.</p>\r\n");
    data->body = strcat(data->body, "\t</body>\r\n");
    data->body = strcat(data->body, "</html>\r\n");
}

int http_download(Ack_Data *data) {
    if (!data->fp) {
        data->fp = fopen(data->path, "r");
    }
    data->status_code = 200;
    data->status_line = "HTTP/1.1 200 OK\r\n";
    data->header = malloc(1024);
    data->body = malloc(80 * 1024);
    fread(data->body, 1, 80 * 1024, data->fp);
    mkheader(data);
    if (feof(data->fp)) {
        fclose(data->fp);
        return 1;
    }
    return 0;
}

int http_chunk(Ack_Data *data) {
    if (!data->fp) {
        data->fp = fopen(data->path, "r");
    }
    data->status_code = 200;
    data->status_line = "HTTP/1.1 200 OK\r\n";
    data->header = malloc(1024);
    data->body = malloc(80 * 1024);
    fread(data->body, 1, 80 * 1024, data->fp);
    mkheader(data);
    if (feof(data->fp)) {
        fclose(data->fp);
        return 1;
    }
    return 0;
}