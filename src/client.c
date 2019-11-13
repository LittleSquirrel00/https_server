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

void server_msg_cb(struct bufferevent *bev, void *arg) {
    char msg[1024];
    int len = bufferevent_read(bev, msg, sizeof(msg));
    printf("Recv %d bytes messsage from server:\n %s\n", len, msg);
    SSL *ssl = bufferevent_openssl_get_ssl(bev);
    // int state = SSL_get_shutdown(ssl);
    // if (state == 0) SSL_set_shutdown(ssl, SSL_RECEIVED_SHUTDOWN);
    SSL_shutdown(ssl);
    struct event_base *base = bufferevent_get_base(bev);
    event_base_loopexit(base, NULL);
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

    bufferevent_free(bev);

    // free event_cmd
    // need struct as event is defined as parameter
    struct event *ev = (struct event *)arg;
    event_free(ev);
}

int main()
{
    // build the message to be sent
    int length = 800; // the size of message
    char* mesg = (char*)malloc((length+1)*sizeof(char)); // Look out the end mark '/0' of a C string
    if (mesg == NULL)
        exit(1);
    int i;
    for (i=0; i<length; i++) 
        strcat(mesg, "a");
 
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    SSL *ssl = SSL_new(ctx);

    // build socket
    int port = 8088;
    struct sockaddr_in my_address;
    memset(&my_address, 0, sizeof(my_address));
    my_address.sin_family = AF_INET;
    my_address.sin_addr.s_addr = htonl(0x7f000001); // 127.0.0.1
    my_address.sin_port = htons(port);
 
    // build event base
    struct event_base* base = event_base_new();
 
    // set TCP_NODELAY to let data arrive at the server side quickly
    evutil_socket_t fd;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    struct bufferevent* conn = bufferevent_openssl_socket_new(base, fd, ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE);
    // struct bufferevent* conn = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
    int enable = 1;
    // if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable)) < 0)
    //    printf("ERROR: TCP_NODELAY SETTING ERROR!\n");
    bufferevent_setcb(conn, server_msg_cb, NULL, event_cb, NULL); // For client, we don't need callback function
    bufferevent_enable(conn, EV_WRITE|EV_READ);
    if(bufferevent_socket_connect(conn,(struct sockaddr*)&my_address,sizeof(my_address)) == 0)
        printf("connect success\n");
 
    // start to send data
    bufferevent_write(conn, mesg, length);
    // check the output evbuffer
    struct evbuffer* output = bufferevent_get_output(conn);
    int len = 0;
    len = evbuffer_get_length(output);
    printf("output buffer has %d bytes left\n", len);
 
    event_base_dispatch(base);
    
    free(mesg);
    mesg = NULL;
 
    bufferevent_free(conn);
    event_base_free(base);
    SSL_CTX_free(ctx);

    printf("Client program is over\n");
 
    return 0;
}