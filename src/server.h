#ifndef SERVER_H_INCLUDED_
#define SERVER_H_INCLUDED_

// 分块传输控制块
struct HTTP_Chunk {
    struct bufferevent *bev;
    char *HTTP_msg;
    struct event *timer;
};

typedef struct {
    pthread_t tid;
    struct event_base *base;
    struct bufferevent *bev;
} Buffer_Thread;

typedef struct {
    pthread_t tid;
    struct event_base *base;
} Listener_Thread;

typedef struct {
    pthread_t tid;
    struct event_base *base;
    struct evbuffer *evb;
} IO_Thread;

/** 创建bufferevent的线程池，用于处理SSL连接和读写数据
 *  实现：
 *  参数：
 *      int threadnums: 线程池中线程数量
 *  返回值：
 *      Bufferevent线程池指针
 */
static Buffer_Thread *Create_Buffer_Thread_Pool(int threadnums);

/** 初始化Bufferevent子线程
 *  实现：
 *      创建event_base事件循环
 *      创建定时器事件做守护进程，防止事件循环退出
 */
static void *buffer_workers(void *arg);

/** 守护进程，空函数，防止Bufferevent线程的事件循环自动终止
 */
static void buffer_cb(evutil_socket_t fd, short events, void *arg);

/** 创建文件读写IO的线程池，用于处理上传或下载文件
 *  实现：
 *  参数：
 *      int threadnums: 线程池中线程数量
 *  返回值：
 *      IO线程池指针
 */
static IO_Thread *Create_IO_Thread_Pool(int threadnums);

/** 初始化IO子线程
 *  实现：
 *      创建event_base事件循环
 *      创建定时器事件做守护进程，防止事件循环退出
 */
static void *io_workers(void *arg);

/** 守护进程，空函数，防止IO线程的事件循环自动终止
 */
static void io_cb(evutil_socket_t fd, short events, void *arg);

/** OpenSSL初始化程序
 *  实现：
 *      初始化SSL库、加密算法、错误消息
 *      生成服务器ctx
 *      加载并校验证书和密钥  
 *  参数：
 *      char *cert_file: 证书的路径
 *      char *priv_file: 密钥的路径
 *  返回值：
 *      服务器ctx
 */
static SSL_CTX* Init_OpenSSL(char *cert_file, char *priv_file);

/** https listener回调函数
 *  实现：
 *      接受TCP连接
 *      生成SSL，与客户端建立SSL连接
 *      创建libevent，利用libevent处理消息
 *      设置libevent读、写、事件回调函数和超时
 *      默认长连接、非管道化
 *  TODO: 管道
 */
static void https_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ctx);

/** http listener回调函数
 *  实现：
 *      接受TCP连接
 *      创建libevent，利用libevent处理消息
 *      设置libevent读、写、事件回调函数和超时
 *      默认长连接、非管道化
 *  TODO: 管道
 */
static void http_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ctx);

/** listener的事件回调函数
 *  实现：
 *     处理监听器意外事件，断开连接，释放事件资源，终止服务运行
 */
static void accept_error_cb(struct evconnlistener *listener, void *ctx);

/** 分块传输读取动态内容
 *  TODO:读取方式
 */
static int chunk_read(char *msg, char *buf, int *eof);

/** 利用定时器事件进行分块传输
 *
 */
static void chunk_timer_cb(evutil_socket_t fd, short events, void *arg);

/** bufferevent的读回调函数
 *  实现：
 *      从读缓冲区读HTTP报文
 *      TODO: 解析HTTP请求报文
 *      TODO: 生成HTTP应答报文
 *      TODO: 分块传输/管道
 *      将HTTP应答报文写入写缓冲区
 *      是否使用长连接，KEEP-ALIVE为2s(超时断开连接)，短连接直接断开连接释放资源
 */
static void read_cb(struct bufferevent *bev, void *ctx);

/** bufferevent的事件回调函数
 *  实现：
 *      事件类型：BEV_EVENT_EOF:4,BEV_EVENT_ERROR:5,BEV_EVENT_TIMEOUT:6,BEV_EVENT_CONNECTED:7
 *      读写事件：BEV_EVENT_READING:0,BEV_EVENT_WRITING:1
 *      CONNECTED事件表示成功建立连接
 *      其他事件断开连接，释放事件资源
 */
static void event_cb(struct bufferevent *bev, short events, void *ctx);

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

#endif 