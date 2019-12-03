#ifndef PARSER_H_DEFINDED_
#define PARSER_H_DEFINDED_

typedef struct {
    char *home;
    char *url;
    char *path;
    char *argv;
} Ack_URL;

#define SSET "set"
#define SUNSET "unset"
#define ISET -1
#define IUNSET -2

typedef enum {GET, CHUNK, UPLOAD, DOWNLOAD} Down_Up_Load;

#define URL_SIZE 256
#define BUFFER_SIZE 512
#define HEADER_SIZE 1024
#define BLOCK_SIZE 256 * 1024

typedef struct {
    char *status_line;
    char *header;
    char *body;
    
    char *home;
    char *url;
    char *path;
    char *url_argv;

    struct stat *st;
    FILE *fp;
    int fd;
    
    int content_length;
    char *content_type;
    char *boundary;
    int status_code;

    Down_Up_Load load;
} Ack_Data;

int on_message_begin(http_parser *parser);

int on_headers_complete(http_parser *parser);

int on_url(http_parser *parser, const char *at, size_t length);

int on_status(http_parser *parser, const char *at, size_t length);

int on_header_field(http_parser *parser, const char *at, size_t length);

int on_header_value(http_parser *parser, const char *at, size_t length);

int on_body(http_parser *parser, const char *at, size_t length);

int on_message_complete(http_parser *parser);

int on_chunk_header(http_parser *parser);

int on_chunk_complete(http_parser *parser);

int http_upload(http_parser *parser);

int http_download(http_parser *parser);

int http_chunk(http_parser *parser);

void data_init(Ack_Data *data);

void data_free(Ack_Data *data);

#endif
