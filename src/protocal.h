#ifndef PROTOCAL_H_DEFINDED_
#define PROTOCAL_H_DEFINDED_

// 状态码
#define 	HTTP_OK                 200     // request completed ok
#define 	HTTP_NOCONTENT          204     // request does not have content
#define 	HTTP_MOVEPERM           301     // the uri moved permanently
#define 	HTTP_MOVETEMP           302     // the uri moved temporarily
#define 	HTTP_NOTMODIFIED        304     // page was not modified from last
#define     HTTP_BADREQUEST         400     // invalid http request was made
#define 	HTTP_NOTFOUND           404     // could not find content for uri
#define 	HTTP_BADMETHOD          405     // method not allowed for this uri
#define 	HTTP_ENTITYTOOLARGE     413     //
#define 	HTTP_EXPECTATIONFAILED  417     // we can't handle this expectation
#define 	HTTP_INTERNAL           500     // internal error
#define 	HTTP_NOTIMPLEMENTED     501     // not implemented
#define 	HTTP_SERVUNAVAIL        503     // the server is not available

// 请求类型
enum http_req_type { REQ_GET = 1 << 0, REQ_POST = 1 << 1, REQ_HEAD = 1 << 2, 
    REQ_PUT = 1 << 3, REQ_DELETE = 1 << 4, REQ_OPTIONS = 1 << 5, 
    REQ_TRACE = 1 << 6, REQ_CONNECT = 1 << 7, REQ_PATCH = 1 << 8};

// 错误类型
enum http_request_error {REQ_HTTP_TIMEOUT, REQ_HTTP_EOF, REQ_HTTP_INVALID_HEADER, 
    REQ_HTTP_BUFFER_ERROR, REQ_HTTP_REQUEST_CANCEL, REQ_HTTP_DATA_TOO_LONG};

enum http_parser_code { HTTP_KEEP_ALIVE = 1 << 0, HTTP_REQ_CLOSE = 1 << 1,
    HTTP_DOWNLOAD = 1 << 2, HTTP_UPLOAD = 1 << 3, HTTP_CHUNKED = 1 << 4, HTTP_BAD_REQ = 1 << 5};

// http请求包/应答包
enum http_request_kind { HTTP_REQUEST, HTTP_RESPONSE};

typedef struct {
    enum http_req_type type;
    char *url;
    char *version;
    char *header;
    char *body;
} HTTP_Req;

typedef struct {
    enum http_req_type type;
    char *version;
    short status_code;
    char *state;
    char *header;
    char *body;
    short parser_code;
} HTTP_Ack;


/** TODO: 实现HTTP解析
 *  实现：
 *      解析HTTP，确认返回编码    
 *  参数：
 *      short *msg: SSL解码后的消息字符串
 *      char *buf: HTTP处理后返回的消息字符串
 *  返回值：
 *      short:   GET、POST、上传、下载、KEEP-ALIVE、Req-Close、Chunk、出错
 *      默认值：   0    0      0    0       1           0       0     0
 *      0b00001000
 *  TODO: HTTP状态码
 *  TODO: GET、POST报文解析，应答报文
 *  TODO: 上传、下载文件
 */
short HTTP_Parser(char *msg, char *buf);

HTTP_Req *http_req_parser(struct evbuffer *msg);

HTTP_Ack *http_create_ack(HTTP_Req *req);

#endif