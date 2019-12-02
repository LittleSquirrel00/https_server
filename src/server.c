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
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <http_parser.h>
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
#include "server.h"
#include "sthread.h"
#include "parser.h"

extern Buffer_Thread *buffer_threads;
extern int BUFFER_THREAD_NUMS;
extern int buffer_thread_index;
extern IO_Thread *io_threads;
extern int IO_THREAD_NUMS;
extern int io_thread_index;

static char *HOME_DIR = "./home";

int main(int argc, char **argv) {
    // 服务器配置
    BUFFER_THREAD_NUMS = get_nprocs();
    IO_THREAD_NUMS = 2;

    Listener_Thread *listener_thread = (Listener_Thread *)malloc(sizeof(Listener_Thread));
    struct event_base *base;
    struct evconnlistener *http_listener, *https_listener;
    struct sockaddr_in http_sin, https_sin;

    int http_port = 8080;
    int https_port = 443;

    // 初始化openssl
    char *cert = argc > 1 ? argv[1] : "cacert.pem";
    char *priv = argc > 2 ? argv[2] : "private_key.pem";
    SSL_CTX *ctx = Init_OpenSSL(cert, priv);

    // 设置地址sockaddr
    memset(&http_sin, 0, sizeof(http_sin));
    http_sin.sin_family = AF_INET;           // This is an INET address
    http_sin.sin_addr.s_addr = htonl(0);     // Listen on 0.0.0.0
    http_sin.sin_port = htons(http_port);    // Listen on the given port.
    memset(&https_sin, 0, sizeof(https_sin));
    https_sin.sin_family = AF_INET;           // This is an INET address
    https_sin.sin_addr.s_addr = htonl(0);     // Listen on 0.0.0.0
    https_sin.sin_port = htons(https_port);   // Listen on the given port.

    evthread_use_pthreads();
    buffer_threads = Create_Buffer_Thread_Pool(BUFFER_THREAD_NUMS);
    io_threads = Create_IO_Thread_Pool(IO_THREAD_NUMS);
    printf("Create %d buffer threads and %d io threads\n", BUFFER_THREAD_NUMS, IO_THREAD_NUMS);
    buffer_thread_index = 0;
    io_thread_index = 0;

    // 创建事件循环
    base = event_base_new();
    if (!base) {
        puts("Couldn't open event base");
        return 1;
    }
    
    listener_thread->tid = pthread_self();
    listener_thread->base = base;

    // 创建listener,绑定监听地址
    http_listener = evconnlistener_new_bind(base, http_accept_cb, NULL,
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, (struct sockaddr*)&http_sin, sizeof(http_sin));
    if (http_listener) {
        printf("Listen to %s:%d\n", inet_ntoa(http_sin.sin_addr), ntohs(http_sin.sin_port));
        evconnlistener_set_error_cb(http_listener, accept_error_cb);
    }
    https_listener = evconnlistener_new_bind(base, https_accept_cb, (void *)ctx,
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, (struct sockaddr*)&https_sin, sizeof(https_sin));
    if (https_listener) {
        printf("Listen to %s:%d\n", inet_ntoa(https_sin.sin_addr), ntohs(https_sin.sin_port));
        evconnlistener_set_error_cb(https_listener, accept_error_cb);
    }
    if (!http_listener && !https_listener) {
        perror("Couldn't create listener!");
        return 1;
    }
    
    // 开始执行事件循环、检测事件, 运行服务器
    event_base_dispatch(base);

    for (int i = 0; i < IO_THREAD_NUMS; i++) {
        pthread_join(io_threads[i].tid, NULL);
    }
    for (int i = 0; i < get_nprocs(); i++) {
        pthread_join(buffer_threads[i].tid, NULL);
    }

    // 结束服务，释放资源
    evconnlistener_free(http_listener);
    evconnlistener_free(https_listener);
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

static void http_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ctx) {
    struct sockaddr_in *sock = (struct sockaddr_in *)addr;
    printf("Got a http accept from %s:%d!\n", inet_ntoa(sock->sin_addr), ntohs(sock->sin_port));
    // 创建bufferevent,设置回调函数
    struct event_base *base = buffer_threads[buffer_thread_index].base;
    buffer_thread_index = (buffer_thread_index + 1) % BUFFER_THREAD_NUMS;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    // 设置超时断开连接
    struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
    evutil_timerclear(tv);
    tv->tv_sec = 2;
    bufferevent_set_timeouts(bev, tv, NULL);
}

static void https_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ctx) {
    struct sockaddr_in *sock = (struct sockaddr_in *)addr;
    printf("Got a https accept from %s:%d!\n", inet_ntoa(sock->sin_addr), ntohs(sock->sin_port));

    // 创建ssl
    SSL_CTX *server_ctx = (SSL_CTX *)ctx;
    SSL *client_ssl = SSL_new(server_ctx);

    // 创建bufferevent,设置回调函数
    struct event_base *base = buffer_threads[buffer_thread_index].base;
    buffer_thread_index = (buffer_thread_index + 1) % BUFFER_THREAD_NUMS;
    struct bufferevent *bev = bufferevent_openssl_socket_new(base, fd, client_ssl, BUFFEREVENT_SSL_ACCEPTING, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_openssl_set_allow_dirty_shutdown(bev, 1); // 允许因网络等问题意外断开连接
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    // 设置超时断开连接    
    struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
    evutil_timerclear(tv);
    tv->tv_sec = 2;
    bufferevent_set_timeouts(bev, tv, NULL);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. Shutting down.\n", 
        err, evutil_socket_error_to_string(err));
    event_base_loopexit(base, NULL);
}

static void http_chunk_cb(evutil_socket_t fd, short events, void *arg) {
    Thread_Argv *argv = (Thread_Argv *)arg;
    struct bufferevent *bev = argv->bev;
    Ack_Data *data = argv->data;
    int finish = http_chunk(data);

    struct evbuffer *output = evbuffer_new();
    evbuffer_add_printf(output, "%s", data->status_line);
    evbuffer_add_printf(output, "%s\r\n", data->header);
    evbuffer_add_printf(output, "%s", data->body);  
    bufferevent_write_buffer(bev, output);

    if (finish) {
        bufferevent_free(bev);
        free(data);
        free(arg);
    }
    else {
        struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_usec = 1000 * 100;
        evtimer_add(argv->ev, &tv);
    }
    free(output);
}

static void io_event_cb(evutil_socket_t fd, short events, void *arg) {
    Thread_Argv *argv = (Thread_Argv *)arg;
    struct bufferevent *bev = argv->bev;
    Ack_Data *data = argv->data;
    int finish = http_download(data);

    struct evbuffer *output = evbuffer_new();
    evbuffer_add_printf(output, "%s", data->status_line);
    evbuffer_add_printf(output, "%s\r\n", data->header);
    evbuffer_add_printf(output, "%s", data->body);  
    bufferevent_write_buffer(bev, output);

    if (finish) {
        bufferevent_free(bev);
        free(data);
        free(arg);
    }
    else {
        struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_usec = 0;
        evtimer_add(argv->ev, &tv);
    }
    free(output);
}

static void read_cb(struct bufferevent *bev, void *ctx) {
    // 从读缓冲区取出数据
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = evbuffer_new();
    int len = evbuffer_get_length(input);
    char *msg = (char *)malloc((len + 1) * sizeof(char));
    memset(msg, 0, (len + 1) * sizeof(char));
    int size = evbuffer_remove(input, msg, len);

    // HTTP解析
    http_parser_settings parser_set;
    http_parser_settings_init(&parser_set);
    parser_set.on_message_begin = on_message_begin;
    parser_set.on_url = on_url;
    parser_set.on_body = on_body;

    http_parser *parser = (http_parser *)malloc(sizeof(http_parser));
    http_parser_init(parser, HTTP_REQUEST);
    
    Ack_Data *data = malloc(sizeof(Ack_Data));
    memset(data, 0, sizeof(Ack_Data));
    data->home = HOME_DIR;
    parser->data = data;
    
    int parser_len = http_parser_execute(parser, &parser_set, msg, len);
    
    if (parser_len != len) {  
        free(msg);
        free(parser);
        bufferevent_free(bev);
        return ;
    }

    if (data->load == DOWNLOAD) {
        Thread_Argv *argv = (Thread_Argv *)malloc(sizeof(Thread_Argv));
        argv->bev = bev;
        argv->data = data;

        struct event_base *base = io_threads[io_thread_index].base;
        io_thread_index = (io_thread_index + 1) % IO_THREAD_NUMS;
        struct event * io_event = event_new(base, -1, EV_TIMEOUT|EV_PERSIST, io_event_cb, argv);
        io_threads[io_thread_index].io_event = io_event;
        struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_usec = 0;
        evtimer_add(io_event, &tv);
    }
    else if (data->load == CHUNK) {
        Thread_Argv *argv = (Thread_Argv *)malloc(sizeof(Thread_Argv));
        argv->data = data;
        argv->bev = bev;
        struct event_base *base = bufferevent_get_base(bev);
        struct event *timer = event_new(base, -1, EV_TIMEOUT, http_chunk_cb, argv);
        argv->ev = timer;

        // 定时器
        struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
        evutil_timerclear(tv);
        tv->tv_usec = 1000 * 100; // 100ms
        evtimer_add(timer, tv);
    }

    // 向写缓冲区写入数据
    evbuffer_add_printf(output, "%s", data->status_line);
    evbuffer_add_printf(output, "%s\r\n", data->header);
    if (data->body) evbuffer_add_printf(output, "%s", data->body);  

    bufferevent_write_buffer(bev, output);

    evbuffer_free(output);
    // 非持久连接
    if (!http_should_keep_alive(parser)) {
        bufferevent_free(bev);
    }
}

static void event_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT)) {
        bufferevent_free(bev);
    }
}
