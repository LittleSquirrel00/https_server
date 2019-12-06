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

int on_url(http_parser *parser, const char *at, size_t length);

int on_header_field(http_parser *parser, const char *at, size_t length);

int on_header_value(http_parser *parser, const char *at, size_t length);

int on_headers_complete(http_parser *parser);

int on_body(http_parser *parser, const char *at, size_t length);

/***************************************************/

int http_upload(http_parser *parser);

int http_download(http_parser *parser);

int http_chunk(http_parser *parser);

/***************************************************/

Ack_Data *data_init(Ack_Data *data);

void data_free(Ack_Data *data);

/** 解析路径，判断执行操作
 *  实现：
 *      判断路径是否存在，是否具有读写权限
 *      判断分块传输，文件大小
 */
static void path_parse(http_parser *parser);

/** 读取目录，生成html页面
 */
static void read_directory(http_parser *parser);

static void read_file(http_parser *parser);


/** 上传小文件
 *  实现：
 *      根据boundary确定传输的数据
 */
static void post_file(http_parser *parser);

/***************************************************/

/** 生成回答报文的http header
 */
static void mkheader(http_parser *parser);

static void header_set_server(char *header);

static void header_set_date(char *header);

static void header_set_content_type(char *header, http_parser *parser);

/** 设置回答报文的body长度
 *  实现：
 *      分块传输不需要设置
 *      下载大文件时返回文件大小
 *      其他返回body大小
 */
static void header_set_content_length(char *header, http_parser *parser);

static void header_set_chunked(char *header, http_parser* parser);

/** 确定是否是持久连接
 */
static void header_set_connection(char *header, http_parser *parser);

/***************************************************/

/** 确认是否使用分块传输
 *  实现： 
 *      是否是dynamic/目录下文件
 *      其他情况待定
 */
static int ischunk(char *path);

int http_body_final(http_parser *parser);

/** 确定是否需要额外io线程(下载/分块)
 */
int need_thread(http_parser *parser);

#endif
