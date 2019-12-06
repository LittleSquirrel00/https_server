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

typedef enum {UNACTION, PULL_DIR, PULL_FILE, PUSH_FILE, UPLOAD_FILE, DOWNLOAD_FILE, CHUNK, ERROR} Action;

#define URL_SIZE 256
#define BUFFER_SIZE 512
#define HEADER_SIZE 1024
#define BLOCK_SIZE 4 * 1024

typedef struct {
    char *status_line;
    char *header;
    char *body;
    
    char *home;
    char *url_argv;
    char *path;
    char *filename;

    struct stat *st;
    FILE *fp;
    int fd;
    
    char *boundary;
    char *content_type;
    int content_length;
    int status_code;
    int read_body_cnt;
    Action act;
} Ack_Data;

int on_message_begin(http_parser *parser);

int on_url(http_parser *parser, const char *at, size_t length);

int on_status(http_parser *parser, const char *at, size_t length);

int on_header_field(http_parser *parser, const char *at, size_t length);

int on_header_value(http_parser *parser, const char *at, size_t length);

int on_headers_complete(http_parser *parser);

int on_body(http_parser *parser, const char *at, size_t length);

int on_message_complete(http_parser *parser);

int on_chunk_header(http_parser *parser);

int on_chunk_complete(http_parser *parser);

/***************************************************/

int http_upload(http_parser *parser);

int http_download(http_parser *parser);

int http_chunk(http_parser *parser);

/***************************************************/

Ack_Data *data_init(Ack_Data *data);

void data_free(Ack_Data *data);

static void path_parse(http_parser *parser);

static void read_directory(http_parser *parser);

static void read_file(http_parser *parser);

static void post_file(http_parser *parser);

/***************************************************/

static void mkheader(http_parser *parser);

static void header_set_server(char *header);

static void header_set_date(char *header);

static void header_set_content_type(char *header, http_parser *parser);

static void header_set_content_length(char *header, http_parser *parser);

static void header_set_chunked(char *header, http_parser* parser);

static void header_set_connection(char *header, http_parser *parser);

/***************************************************/

static int ischunk(char *path);

int http_body_final(http_parser *parser);

int need_thread(http_parser *parser);

#endif
