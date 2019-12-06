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
#include "html.h"

/***************************************************/
int on_message_begin(http_parser *parser) {
    return 0;
}

int on_url(http_parser *parser, const char *at, size_t length) { 
    Ack_Data *data = (Ack_Data *)parser->data;
    memset(data->path, 0, URL_SIZE);
    sprintf(data->path, "%s%.*s", data->home, (int)length, at);
    if (HTTP_GET == parser->method) {
        if (data->url_argv = strchr(data->path, '?')) {
            *(data->url_argv) = '\0';
            data->url_argv++;
            // argv_parse
        }
        path_parse(parser);
        mkheader(parser);
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
            data->act = PUSH_FILE;
        }
    }
    else if (data->content_length == ISET) {
        char tmp[32];
        sprintf(tmp, "%.*s", (int)length, at);
        data->content_length = atoi(tmp);
        if (!data->content_length) {
            path_parse(parser);
            mkheader(parser);
        }
    }
    return 0;
}

int on_headers_complete(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (!data->boundary) {
        path_parse(parser);
        mkheader(parser);
    }
    data->read_body_cnt = 0;
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) {
    Ack_Data *data = (Ack_Data *)parser->data;
    data->read_body_cnt = length;
    sprintf(data->body, "%.*s", (int)length, at);
    
    char *fbegin = strstr(data->body, "filename=") + 10;
    char *fend = strstr(fbegin, "\"");
    data->filename = malloc(128);
    strcpy(data->filename, data->path);
    strncat(data->filename, fbegin, (int)(fend - fbegin));

    data->act = PUSH_FILE;
    path_parse(parser);
    
    if (data->status_code != 200) {
        mkheader(parser);
        data->act = ERROR;
    }
    else {
        if (length <= data->content_length) {
            post_file(parser);
        }
        else {
            data->act = UPLOAD_FILE;
        }
    }
    return 0;
}

/***************************************************/
Ack_Data *data_init(Ack_Data *data) {
    data = malloc(sizeof(Ack_Data));
    memset(data, 0, sizeof(Ack_Data));
    data->header = malloc(HEADER_SIZE);
    data->body = malloc(BLOCK_SIZE);
    data->path = malloc(URL_SIZE);

    data->content_length = IUNSET;
    data->fd = -1;
    data->status_code = 200;
    data->act = UNACTION;
    return data;
}

void data_free(Ack_Data *data) {
    free(data->header);
    free(data->body);
    free(data->path);
    free(data);
}

void mkheader(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    memset(data->header, 0, HEADER_SIZE);
    header_set_server(data->header);
    header_set_date(data->header);
    header_set_content_type(data->header, parser);
    header_set_content_length(data->header, parser);
    header_set_chunked(data->header, parser);   
    header_set_connection(data->header, parser);
}

static void header_set_server(char *header) {
    strcat(header, "Server: HTTP-Server/0.1\r\n");
}

static void header_set_date(char *header) {
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    char buf[BUFFER_SIZE];
    strftime(buf, BUFFER_SIZE, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
    strcat(header, buf);
}

static void header_set_content_type(char *header, http_parser *parser) {
    Ack_Data  *data = parser->data;
    char *mime = NULL;
    if (data->act == DOWNLOAD_FILE) {
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

    char ct[BUFFER_SIZE];
    sprintf(ct, "Content-Type: %s; charset=utf-8\r\n", mime);
    strcat(header, ct);
}

static void header_set_content_length(char *header, http_parser *parser) {
    Ack_Data *data = parser->data;
    char len[32];
    if (data->act != CHUNK) {
        sprintf(len, "Content-Length: %d\r\n", (data->act == DOWNLOAD_FILE ? (int)data->st->st_size : (int)strlen(data->body)));
        strcat(header, len);
    }
}

static void header_set_chunked(char *header, http_parser *parser) {
    Ack_Data *data = parser->data;
    if (data->act == CHUNK)
        strcat(header, "Transfer-Encoding: chunked\r\n");
}

static void header_set_connection(char *header, http_parser *parser) {
    Ack_Data *data = parser->data;
    int keep = data->act == DOWNLOAD_FILE || data->act == CHUNK || http_should_keep_alive(parser);
    strcat(data->header, keep ? "Connection: keep-alive\r\n" : "Connection: Close\r\n");
}

/***************************************************/

static void path_parse(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    data->st = (struct stat *)malloc(sizeof(struct stat));
    
    if (stat(data->path, data->st) < 0) {
        data->status_code = 404;
        data->status_line = "HTTP/1.1 404 NOT FOUND\r\n";
        free(data->body);
        data->body = HTTP_NotFound();
    }
    else if (!(data->st->st_mode & S_IROTH)) {
        data->status_code = 403;
        data->status_line = "HTTP/1.1 403 FORBIDDEN\r\n";
        free(data->body);
        data->body = HTTP_Forbidden();
    }
    else if (data->act != PUSH_FILE) {
        data->status_code = 200;
        data->status_line = "HTTP/1.1 200 OK\r\n";
        
        if (S_ISDIR(data->st->st_mode)) {
            data->act = PULL_DIR;
            read_directory(parser);
        } 
        else if (ischunk(data->path)) 
            data->act = CHUNK;
        else if (data->st->st_size > BLOCK_SIZE) 
            data->act = DOWNLOAD_FILE;
        else {
            read_file(parser);
            data->act = PULL_FILE;
        }
    }
}

static void read_directory(http_parser *parser) {
    Ack_Data *data = parser->data;
    char buf[1024];
    DIR *d = opendir(data->path);
    struct dirent *dir;

    memset(data->body, 0, BLOCK_SIZE);
    strcat(data->body, "<html>\r\n");
    strcat(data->body, "<head>\r\n");
    strcat(data->body, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n");
    sprintf(buf, "<title>Directory listing for %s</title>\r\n", data->path+6);
    strcat(data->body, buf);
    strcat(data->body, "</head>\r\n");
    strcat(data->body, "<body>\r\n");
    sprintf(buf, "<h1>Directory listing for %s</h1>\r\n", data->path+6);
    strcat(data->body, buf);
    strcat(data->body, "<hr>\r\n");
    strcat(data->body, "<ul>\r\n");
    
    while ((dir = readdir(d)) != NULL) {
        if (!strcmp(dir->d_name , ".") || !strcmp(dir->d_name, "..")) continue;
        char *ch = dir->d_type == DT_DIR ? "/" : "";
        sprintf(buf, "<li><a href=\"%s%s\">%s%s</a></li>\r\n", dir->d_name, ch, dir->d_name, ch); 
        strcat(data->body, buf);
    }
    closedir(d);
    
    strcat(data->body, "</ul>\r\n");
    strcat(data->body, "<hr>\r\n");
    strcat(data->body, "<form action=\"/upload/\" method=\"post\" enctype=\"multipart/form-data\">\r\n");
    strcat(data->body, "<p><input type=\"file\" name=\"upload\"></p>\r\n");
    strcat(data->body, "<p><input type=\"submit\" value=\"submit\"></p>\r\n");
    strcat(data->body, "</form>\r\n");
    strcat(data->body, "</body>\r\n");
    strcat(data->body, "</html>\r\n");
}

static void read_file(http_parser *parser) {
    Ack_Data *data = parser->data;
    memset(data->body, 0, BLOCK_SIZE);
    FILE *fp = fopen(data->path, "rb");
    int size = fread(data->body, 1, BLOCK_SIZE, fp);
    fclose(fp);    
}

static void post_file(http_parser *parser) {
    Ack_Data *data = parser->data;
    FILE *fp = fopen(data->filename, "w");

    int len = strlen(data->boundary);
    char *boundary_end = malloc(len + 3);
    strcpy(boundary_end, data->boundary);
    strcat(boundary_end, "--");
    char *end = strstr(data->body, boundary_end);
    char *bbegin = strstr(data->body, data->boundary);

    while (bbegin && bbegin != end) {
        char *dbegin = strstr(bbegin, "\r\n\r\n") + 4;
        char *bend = strstr(dbegin, data->boundary);
        fwrite(dbegin, 1, (int)(bend - dbegin - 2), fp);
        bbegin = bend;
    }
    fclose(fp);
    
    data->status_code = 200;
    data->status_line = "HTTP/1.1 200 OK\r\n";
    read_directory(parser);
    mkheader(parser);
}

/***************************************************/
int http_upload(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (!data->fp) data->fp = fopen(data->filename, "w");
    int len = strlen(data->boundary);
    char *boundary_end = malloc(len + 3);
    strcpy(boundary_end, data->boundary);
    strcat(boundary_end, "--");
    char *end = strstr(data->body, boundary_end);

    char *bbegin = strstr(data->body, data->boundary);
    while (bbegin && bbegin != end) {
        char *dbegin = strstr(bbegin, "\r\n\r\n") + 4;
        char *bend = strstr(dbegin, data->boundary);
        fwrite(dbegin, 1, (int)(bend - dbegin - 2), data->fp);
        bbegin = bend;
    }
    
    if (end) {
        fclose(data->fp);
        data->status_code = 200;
        data->status_line = "HTTP/1.1 200 OK\r\n";
        path_parse(parser);
        mkheader(parser);
        return 1;            
    }
    return 0;
}

int http_download(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (!data->fp) data->fp = fopen(data->path, "r");
    memset(data->body, 0, BLOCK_SIZE);
    
    if (!fread(data->body, 1, BLOCK_SIZE, data->fp)) {
        fclose(data->fp);
        return 1;
    }
    return 0;
}

int http_chunk(http_parser *parser) {
    Ack_Data *data = (Ack_Data *)parser->data;
    if (!data->fp) data->fp = fopen(data->path, "r");
    memset(data->body, 0, BLOCK_SIZE);
    if (!fgets(data->body, BLOCK_SIZE, data->fp)) {
        fclose(data->fp);
        return 1;
    }
    return 0;
}

int ischunk(char *path) {
    return strstr(path, "dynamic") != NULL;
}

int need_thread(http_parser *parser) {
    Ack_Data *data = parser->data;
    return data->act == DOWNLOAD_FILE || data->act == CHUNK;
}

int http_body_final(http_parser *parser) {
    Ack_Data *data = parser->data;
    return data->content_length <= data->read_body_cnt;
}