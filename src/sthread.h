#ifndef STHREAD_H_DEFINED_
#define STHREAD_H_DEFINDED_

#include <http_parser.h>
#include "parser.h"

// bufferevent线程参数
typedef struct {
    pthread_t tid;
    struct event_base *base;
    struct bufferevent *bev;
} Buffer_Thread;

// listener线程参数
typedef struct {
    pthread_t tid;
    struct event_base *base;
} Listener_Thread;

// io线程参数
typedef struct {
    pthread_t tid;
    struct event_base *base;
    struct event *io_event;
} IO_Thread;

typedef struct {
    struct bufferevent *bev;
    Ack_Data *data;
    struct event *ev;
    struct timeval tv;
} Thread_Argv;

/** 创建bufferevent的线程池，用于处理SSL连接和读写数据
 *  实现：
 *  参数：
 *      int threadnums: 线程池中线程数量
 *  返回值：
 *      Bufferevent线程池指针
 */
Buffer_Thread *Create_Buffer_Thread_Pool(int threadnums);

/** 初始化Bufferevent子线程
 *  实现：
 *      创建event_base事件循环
 *      创建定时器事件做守护进程，防止事件循环退出
 */
void *buffer_workers(void *arg);

/** 守护进程，空函数，防止Bufferevent线程的事件循环自动终止
 */
void buffer_cb(evutil_socket_t fd, short events, void *arg);

/** 创建文件读写IO的线程池，用于处理上传或下载文件
 *  实现：
 *  参数：
 *      int threadnums: 线程池中线程数量
 *  返回值：
 *      IO线程池指针
 */
IO_Thread *Create_IO_Thread_Pool(int threadnums);

/** 初始化IO子线程
 *  实现：
 *      创建event_base事件循环
 *      创建定时器事件做守护进程，防止事件循环退出
 */
void *io_workers(void *arg);

/** 守护进程，空函数，防止IO线程的事件循环自动终止
 */
void io_cb(evutil_socket_t fd, short events, void *arg);

#endif
