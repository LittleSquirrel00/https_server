#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include "sthread.h"

Buffer_Thread *buffer_threads;
int BUFFER_THREAD_NUMS;
IO_Thread *io_threads;
int IO_THREAD_NUMS;
int buffer_thread_index;
int io_thread_index;

Buffer_Thread *Create_Buffer_Thread_Pool(int threadnums) {
    Buffer_Thread *threads = (Buffer_Thread *)malloc(threadnums * sizeof(Buffer_Thread));
    for (int i = 0; i < threadnums; i++) {
        if (pthread_create(&threads[i].tid, NULL, buffer_workers, &threads[i])) {
            perror("Create buffer thread pool failed\n");
            exit(1);
        }
    }
    return threads;
}

void buffer_cb(evutil_socket_t fd, short events, void *arg) {
    Buffer_Thread *me = (Buffer_Thread *)arg;
    printf("Current tid: %ld, there are %d events\n", pthread_self(), event_base_get_num_events(me->base, EVENT_BASE_COUNT_ADDED));
}

void *buffer_workers(void *arg) {
    Buffer_Thread *me = (Buffer_Thread *)arg;
    me->base = event_base_new();
    me->tid = pthread_self();
    me->bev = NULL;
    struct event *bufferd = event_new(me->base, -1, EV_TIMEOUT|EV_PERSIST, buffer_cb, me);
    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = 3600;
    evtimer_add(bufferd, &tv);
    event_base_dispatch(me->base);
    event_free(bufferd);
    event_base_free(me->base);
    return NULL;
}

IO_Thread *Create_IO_Thread_Pool(int threadnums) {
    IO_Thread *threads = (IO_Thread *)malloc(threadnums * sizeof(IO_Thread));
    for (int i = 0; i < threadnums; i++) {
        if (pthread_create(&threads[i].tid, NULL, io_workers, &threads[i])) {
            perror("Create io thread failed!\n");
            exit(1);
        }
    }
    return threads;
}

void io_cb(evutil_socket_t fd, short events, void *arg) {
    IO_Thread *me = (IO_Thread *)arg;
    printf("Current tid: %ld, there are %d events\n", pthread_self(), event_base_get_num_events(me->base, EVENT_BASE_COUNT_ADDED));
}

void *io_workers(void *arg) {
    IO_Thread *me = (IO_Thread *)arg;
    me->base = event_base_new();
    me->tid = pthread_self();
    me->evb = NULL;
    struct event *iod = event_new(me->base, -1, EV_TIMEOUT|EV_PERSIST, io_cb, me);
    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = 3600;
    evtimer_add(iod, &tv);
    event_base_dispatch(me->base);
    event_free(iod);
    event_base_free(me->base);
    return NULL;
}