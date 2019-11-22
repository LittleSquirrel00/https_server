#ifndef CLIENT_H_DEFINDED_
#define CLIENT_H_DEFINDED_

// enum MsgType {GET, POST, UPLOAD, DOWNLOAD}; // 请求报文类型

/** bufferevent读回调函数，处理服务器返回的消息
 *  TODO: 服务器管道
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
char *CreateMsg(int msgType, char *uri);

static void timer_cb(evutil_socket_t fd, short events, void *arg);

#endif