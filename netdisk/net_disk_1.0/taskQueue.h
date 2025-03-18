#ifndef __TASKQUEUE_H__
#define __TASKQUEUE_H__
#include "head.h"

typedef struct task_n{
    MYSQL *conn;//子线程自己的连接
    int new_fd;// 新连接的文件描述符
    int user_id;//用户id
    char path[1024];//用户所在路径
    int path_id;//所在目录的id，为了方便操作数据库
    struct task_n* pnext;// 指向下一个任务节点的指针
}task_t;


typedef struct{
    task_t *pFront;//队列头
    task_t *pRear;//队列尾
    pthread_mutex_t mutex;//锁
    int queueSize;//当前任务的个数

}taskQueue_t;

//入队
int taskEnQueue(taskQueue_t *pQueue,int new_fd,int user_id);
//出队
int taskDequeue(taskQueue_t *pQueue,task_t **pnew);
#endif