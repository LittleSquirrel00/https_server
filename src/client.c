/************************************
 * For msmr 
 * server.c
 * tesing the speed of bufferevent_write
 * 2015-02-03
 * author@tom
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
#include <event2/event.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/thread.h>

enum MsgType {GET, POST, UPLOAD, DOWNLOAD}; // 请求报文类型

/** bufferevent读回调函数，处理服务器返回的消息
 *  TODO: 与服务器保持连接，服务器管道，停等协议回复
 *  TODO: 上下文相关（分块传输合并），多次交互
 */
void server_msg_cb(struct bufferevent *bev, void *arg);

/** bufferevent事件回调函数，处理意外事件
 *  实现：
 *      报错，释放连接和资源
 */
void event_cb(struct bufferevent *bev, short event, void *arg);

/** 生成HTTP请求报文，实现GET、POST、上传、下载
 *  参数：
 *      MsgType msgType: 请求报文类型
 *      char *uri: 供上传下载的uri
 *  返回值：
 *      HTTP格式请求报文
 *  TODO:GET
 *  TODO:POST
 *  TODO:上传
 *  TODO:下载
 */
char *CreateMsg(enum MsgType msgType, char *uri);

/**
 *  TODO:HTTP请求报文
 *  TODO:HTTP持久连接，保持连接
 *  TODO:HTTP多次交互，
 */
int main()
{
    // 构建消息
    char *mesg = CreateMsg(GET, NULL);
    int length = strlen(mesg);

    // 初始化OpenSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    SSL *ssl = SSL_new(ctx);

    // 创建socket
    int port = 8088;
    struct sockaddr_in my_address;
    memset(&my_address, 0, sizeof(my_address));
    my_address.sin_family = AF_INET;
    my_address.sin_addr.s_addr = htonl(0x7f000001); // 127.0.0.1
    my_address.sin_port = htons(port);
 
    // 创建事件循环和写入事件
    struct event_base* base = event_base_new();
    evutil_socket_t fd;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    struct bufferevent* conn = bufferevent_openssl_socket_new(base, fd, ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(conn, server_msg_cb, NULL, event_cb, NULL);
    bufferevent_enable(conn, EV_WRITE|EV_READ);
    if(bufferevent_socket_connect(conn,(struct sockaddr*)&my_address,sizeof(my_address)) == 0)
        printf("connect success\n");
 
    // 将数据写入缓冲区
    bufferevent_write(conn, mesg, length);
    
    // 检测写入缓冲区数据
    struct evbuffer* output = bufferevent_get_output(conn);
    int len = 0;
    len = evbuffer_get_length(output);
    printf("output buffer has %d bytes left\n", len);
 
    // 开始执行
    event_base_dispatch(base);
 
    // 释放资源
    bufferevent_free(conn);
    event_base_free(base);
    SSL_CTX_free(ctx);
    // printf("Client program is over\n");
    
    return 0;
}

char *CreateMsg(enum MsgType msgType, char *uri) {
    char *msg = NULL;
    msg = "ABCDTest";
    switch(msgType) {
        case GET: ; break;
        case POST: ; break;
        case UPLOAD: ; break;
        case DOWNLOAD: ; break;
        default: ; break;
    }
    return msg;
}

void event_cb(struct bufferevent *bev, short event, void *arg) {
    if (event & BEV_EVENT_EOF) {
        printf("Connection closed.\n");
    }
    else if (event & BEV_EVENT_ERROR) {
        printf("Some other error.\n");
    }
    else if (event & BEV_EVENT_CONNECTED) {
        printf("Client has successfully cliented.\n");
        return;
    }
    
    struct event *ev = (struct event *)arg;
    bufferevent_free(bev);
    event_free(ev);
}

void server_msg_cb(struct bufferevent *bev, void *arg) {
    char msg[1024];
    int len = bufferevent_read(bev, msg, sizeof(msg));
    printf("Recv %d bytes messsage from server:\n%s\n", len, msg);
    SSL *ssl = bufferevent_openssl_get_ssl(bev);
    // int state = SSL_get_shutdown(ssl);
    // if (state == 0) SSL_set_shutdown(ssl, SSL_RECEIVED_SHUTDOWN);
    SSL_shutdown(ssl);
    struct event_base *base = bufferevent_get_base(bev);
    event_base_loopexit(base, NULL);
}