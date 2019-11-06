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
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/thread.h>

#define MAXBUF 1024

void read_cb(struct bufferevent *bev, void *arg) {
    char buf[MAXBUF] = {0};
    char *ip = (char *) arg;
    bufferevent_read(bev, buf, sizeof(buf));
    const char *p = "";
    bufferevent_write(bev, p, strlen(p) + 1);
}

void write_cb(struct bufferevent *bev, void *arg) {
    ;
}

void event_cb(struct bufferevent *bev, short events, void *arg) {
    char *ip = (char *) arg;
    if (events & BEV_EVENT_EOF) {

    }
    else if (events & BEV_EVENT_ERROR) {

    }
    bufferevent_free(bev);
}

void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, 
    struct sockaddr *addr, int len, void *ptr) {
    // char addr[INET_ADDRSTRLEN];
    // struct sockaddr_in *sin = (struct sockaddr_in *) address;
    // inet_ntop(AF_INET, &sin->sin_addr, addr, INET_ADDRSTRLEN);
    // printf("Accept TCP connection from: %s", addr);

    // struct sockaddr_in *client = (struct sockaddr_in *)addr;
    // struct event_base *base = (struct event_base *)ptr;
    // struct bufferevent *bev;

    // bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    // // bev = bufferevent_openssl_socket_new(base, fd, ssl, state, options);
    // // SSL *bufferevent_openssl_get_ssl(bev);
    // bufferevent_setcb(bev, read_cb, write_cb, event_cb, inet_ntoa(client->sin_addr));
    // bufferevent_enable(bev, EV_READ);

    struct sockaddr_in *client = (struct sockaddr_in *)addr;
    struct bufferevent *bev;
    struct event_base *base;
    SSL_CTX *server_ctx;
    SSL *client_ctx;
    server_ctx = (SSL_CTX *)ptr;
    client_ctx = SSL_new(server_ctx);
    base = evconnlistener_get_base(listener);
    bev = bufferevent_openssl_socket_new(base, fd, client_ctx, BUFFEREVENT_SSL_ACCEPTING, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_enable(bev, EV_READ);
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, inet_ntoa(client->sin_addr));
}

int main(int argc, char **argv) {
    short port = 8088;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    SSL_CTX *ctx;
    
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    ctx = SSL_CTX_new(SSLv23_server_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (SSL_CTX_use_certificate_file(ctx, argv[3], SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, argv[4], SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    
    struct event_base *base = event_base_new();
    // evthread_use_pthreads();
    struct evconnlistener *listener = evconnlistener_new_bind(base, 
        accept_conn_cb, (void *)ctx, LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE|LEV_OPT_THREADSAFE, 
        -1, (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) { return 1; }
    
    event_base_dispatch(base);
    evconnlistener_free(listener);
    event_base_free(base);
    
    SSL_CTX_free(ctx);
    return 0;
}