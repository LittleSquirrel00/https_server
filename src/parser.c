#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <http_parser.h>
#include "parser.h"

void filepath_parse(http_parser *parser);
void notfound(http_parser *parser);
void forbidden(http_parser *parser);
void mkheader(http_parser *parser);
int ischunk(char *path);

int on_message_begin(http_parser *parser) {
    Ack_Data *data = parser->data;
    data->load = GET;
    data_init(parser->data);
    data->content_length = IUNSET;
    data->fd = -1;
    return 0;
}

void data_init(Ack_Data *data) {
    data->header = malloc(HEADER_SIZE);
    data->body = malloc(BLOCK_SIZE);
    data->url = malloc(URL_SIZE);
    data->path = malloc(URL_SIZE);
}

void data_free(Ack_Data *data) {
    free(data->header);
    free(data->body);
    free(data->url);
    free(data->path);
    free(data->url);
}

int on_url(http_parser *parser, const char *at, size_t length) { 
    Ack_Data *data = (Ack_Data *)parser->data;
    memset(data->url, 0, URL_SIZE);
    memset(data->path, 0, URL_SIZE);
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
        mkheader(parser);
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
        data->content_type = SSET;
    }
    if (data->content_length == IUNSET && strstr(tmp, "Content-Length")) {
        data->content_length = ISET;
    }
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (data->content_type == SSET) {
        data->content_type = (char *)malloc(BUFFER_SIZE);
        sprintf(data->content_type, "%.*s", (int)length, at);
        char *begin;
        if (begin = strstr(data->content_type, "boundary=")) {
            begin += 9;
            data->boundary = (char *)malloc(BUFFER_SIZE);
            sprintf(data->boundary, "%.*s", (int)(at + length - begin), begin);
        }
    }
    else if (data->content_length == ISET) {
        char tmp[32];
        memset(tmp, 0, sizeof(tmp));
        sprintf(tmp, "%.*s", (int)length, at);
        data->content_length = atoi(tmp);
        if (!data->content_length) {
            filepath_parse(parser);
            mkheader(parser);
        }
    }
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (!data->boundary) {
        filepath_parse(parser);
        mkheader(parser);
    }
    else {
        data->load = UPLOAD;
        FILE *fp = fopen(data->path, "ab");
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
        mkheader(parser);
    }
    return 0;
}

void mkheader(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    memset(data->header, 0, HEADER_SIZE);
    data->header = strcat(data->header, "Server: HTTP-Server/0.1\r\n");

    char buf[BUFFER_SIZE];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof(buf), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
    data->header = strcat(data->header, buf);
    
    char *mime = NULL;
    if (data->load == DOWNLOAD) {
        mime = "application/octet-stream";
    }
    else if (S_ISREG(data->st->st_mode)) {
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
    else {
        char content_length[128];
        if (data->load == DOWNLOAD) {
            sprintf(content_length, "Content-Length: %ld\r\n", data->st->st_size);
        }
        else if (!data->body)
            sprintf(content_length, "Content-Length: 0\r\n");
        else 
            sprintf(content_length, "Content-Length: %ld\r\n", strlen(data->body));

        data->header = strcat(data->header, content_length);
    }

    if (data->load == DOWNLOAD || data->load == CHUNK || http_should_keep_alive(parser))
        data->header = strcat(data->header, "Connection: keep-alive\r\n");
    else 
        data->header = strcat(data->header, "Connection: Close\r\n");
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
        memset(data->body, 0, BLOCK_SIZE);
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
        
        if (ischunk(data->path)) {
            data->load = CHUNK;
        }
        else if (data->st->st_size > BLOCK_SIZE) {
            data->load = DOWNLOAD;
        }
        else {
            memset(data->body, 0, BLOCK_SIZE);
            FILE *fp = fopen(data->path, "rb");
            int size = fread(data->body, 1, BLOCK_SIZE, fp);
            fclose(fp);
        }
    }
}

void notfound(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    memset(data->body, 0, BLOCK_SIZE);
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
    memset(data->body, 0, BLOCK_SIZE);
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

int http_upload(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (!data->fp)
        data->fp = fopen(data->path, "a");

    int size = fwrite(data->body, 1, strlen(data->body), data->fp);
    
    data->content_length -= size;
    mkheader(parser);

    if (data->content_length <= 0 
        || (data->load == CHUNK) && strstr(data->body, "0\r\n\r\n")) {
        return 1;
    }
    return 0;
}

int http_download(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (!data->fp)
        data->fp = fopen(data->path, "rb");
    memset(data->body, 0, BLOCK_SIZE);
    fread(data->body, 1, BLOCK_SIZE, data->fp);
    
    if (feof(data->fp)) {
        fclose(data->fp);
        return 1;
    }
    return 0;
}

int http_chunk(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    
    if (data->fd == -1)
        data->fd = open(data->path, O_RDONLY);
        
    memset(data->body, 0, BLOCK_SIZE);
    // int size = read(data->fd, data->body, BLOCK_SIZE);
    
    int size = read(data->fd, data->body, 10);
    
    if (strstr(data->body, "0\r\n\r\n") || size == 0) {
        close(data->fd);
        return 1;
    }
    return 0;
}

int ischunk(char *path) {
    if (strstr(path, "dynamic")) {
        return 1;
    }
    else return 0;
}