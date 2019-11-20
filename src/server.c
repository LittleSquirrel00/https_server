/************************************
 * server.c
 * 使用libevent框架，实现HTTPS/HTTP服务器
 * 2019-11-15
 * 作者：张枫、李雪菲、赵挽涛
************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>
#include <event.h>
#include <event2/event.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/thread.h>

// 分块传输控制块
struct Chunk_Data {
    struct bufferevent *bev;
    char **chunk_data;
    struct event *timer;
    int i;
};

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

/** listener回调函数
 *  实现：
 *      接受TCP连接
 *      生成SSL，与客户端建立SSL连接
 *      创建libevent，利用libevent处理消息
 *      设置libevent读、写、事件回调函数和超时
 *      默认长连接、非管道化
 *  TODO: 同时支持SSL加密和未加密
 *  TODO: 管道
 */
static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ctx);

/** listener的事件回调函数
 *  实现：
 *     处理监听器意外事件，断开连接，释放事件资源，终止服务运行
 */
static void accept_error_cb(struct evconnlistener *listener, void *ctx);

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
 *  参数：
 *      char *msg: SSL解码后的消息字符串
 *      char **buf: HTTP处理后返回的消息字符串，可进行分块，实现分块传输
 *  返回值：
 *      int： 0 表示HTTP处理成功；1 表示HTTP处理失败
 *  TODO: HTTP状态码
 *  TODO: GET、POST报文解析，应答报文
 *  TODO: 上传、下载文件
 */
int HTTP_Parser(char *msg, char **buf);

/// TODO: 线程池
int main(int argc, char **argv) {
    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    int port = argc > 1 ? atoi(argv[1]) : 8088;
    if (port<=0 || port>65535) {
        puts("Invalid port");
        return 1;
    }

    // 初始化openssl
    char *cert = argc > 2 ? argv[2] : "cacert.pem";
    char *priv = argc > 3 ? argv[3] : "private_key.pem";
    SSL_CTX *ctx = Init_OpenSSL(cert, priv);

    // 设置地址sockaddr
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;           // This is an INET address
    sin.sin_addr.s_addr = htonl(0);     // Listen on 0.0.0.0
    sin.sin_port = htons(port);         // Listen on the given port.

    // 创建事件循环
    base = event_base_new();
    if (!base) {
        puts("Couldn't open event base");
        return 1;
    }

    // 创建listener
    listener = evconnlistener_new_bind(base, accept_conn_cb, (void *)ctx,
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
        perror("Couldn't create listener");
        return 1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);
    printf("Listen to %s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

    // 开始执行事件循环、检测事件, 运行服务器
    event_base_dispatch(base);

    // 结束服务，释放资源
    evconnlistener_free(listener);
    event_base_free(base);
    SSL_CTX_free(ctx);

    return 0;
}

static SSL_CTX* Init_OpenSSL(char *cert_file, char *priv_file) {
    SSL_CTX *ctx;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx || SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, priv_file, SSL_FILETYPE_PEM) <= 0 || 
        !SSL_CTX_check_private_key(ctx)) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    printf("Set up openssl ok!\nLoaded certificate:%s\nLoaded private key:%s\n", cert_file, priv_file);
    return ctx;
}

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ctx) {
    struct sockaddr_in *sock = (struct sockaddr_in *)addr;
    printf("Got a new connection from %s:%d!\n", inet_ntoa(sock->sin_addr), ntohs(sock->sin_port));

    // 创建ssl
    SSL_CTX *server_ctx = (SSL_CTX *)ctx;
    SSL *client_ssl = SSL_new(server_ctx);

    // 创建bufferevent,设置回调函数
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_openssl_socket_new(base, fd, client_ssl, BUFFEREVENT_SSL_ACCEPTING, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_openssl_set_allow_dirty_shutdown(bev, 1); // 允许因网络等问题意外断开连接
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    // 设置超时断开连接
    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = 2;
    bufferevent_set_timeouts(bev, &tv, NULL);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. Shutting down.\n", 
        err, evutil_socket_error_to_string(err));
    event_base_loopexit(base, NULL);
}

static void chunk_timer_cb(evutil_socket_t fd, short events, void *arg) {
    struct Chunk_Data *chunk = (struct Chunk_Data *)arg;
    struct evbuffer *evb = evbuffer_new();
    /// TODO:判断条件
    if (chunk->i < sizeof(chunk->chunk_data)) {
        evbuffer_add_printf(evb, "%s", chunk->chunk_data[chunk->i]);
        bufferevent_write_buffer(chunk->bev, evb);
        evbuffer_free(evb);
    }
    struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
    evutil_timerclear(tv);
    tv->tv_sec = 1;
    evtimer_add(chunk->timer, tv);
}

static void read_cb(struct bufferevent *bev, void *ctx) {
    struct evbuffer *in = bufferevent_get_input(bev);

    printf("Received %zu bytes\n", evbuffer_get_length(in));
    printf("----- data ----\n");
    printf("%.*s\n", (int)evbuffer_get_length(in), evbuffer_pullup(in, -1));

    // sleep(0.1);

    int keep_alive = 1, conn_close = 0;
    int chunk = 0;
    int pipeline = 0;

    // char *buf = (char *)malloc(1024*sizeof(char));
    // memset(buf, 0, sizeof(buf));
    // if (HTTP_Parser(evbuffer_pullup(in, -1), buf));
    // for (int i = 0; i < 128 && strlen(buf[i]) > 0; i++) {
    //     bufferevent_write_buffer(bev, buf[i]);
    // }

    // 添加定时器事件实现分块传输
    if (keep_alive && chunk) {
        struct Chunk_Data *chunk = (struct Chunk_Data *)malloc(sizeof(struct Chunk_Data));
        struct event_base *base = bufferevent_get_base(bev);
        struct event *chunk_timer = event_new(base, -1, EV_TIMEOUT, chunk_timer_cb, chunk);
        chunk->chunk_data;
        chunk->bev = bev;
        chunk->timer = chunk_timer;
        chunk->i = 0;
        struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
        evutil_timerclear(tv);
        tv->tv_sec = 1;
        evtimer_add(chunk_timer, tv);
    }
    else {
        struct evbuffer *out = evbuffer_new();
        bufferevent_write_buffer(bev, in);
    }

    // 持久连接
    if (!keep_alive || conn_close) bufferevent_free(bev);
}

static void event_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_ERROR)
        perror("Error from bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT)) {
        if (BEV_EVENT_TIMEOUT) printf("timeout");
        if (BEV_EVENT_EOF) printf("eof");
        if (BEV_EVENT_ERROR) printf("error");
        bufferevent_free(bev);
    }
}

int HTTP_Parser(char *msg, char **buf) {
}