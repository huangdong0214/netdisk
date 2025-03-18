#include "factory.h"


//任务入队
int taskEnQueue(taskQueue_t *pQueue,int new_fd,int user_id)//定义任务入队函数，参数包括任务队列指针、连接描述符和用户 ID。
{
    task_t *pnew=(task_t*)calloc(1,sizeof(task_t));//为新的任务节点分配内存，并初始化为 0。
    pnew->new_fd=new_fd;//将传入的连接描述符赋值给新节点。
    pnew->user_id=user_id;//将传入的用户 ID 赋值给新节点。
    static int port=0;//定义静态变量 port 用于记录连接使用的端口号，保证每次调用保留上次的值。
    port++;//将静态变量 port 自增，为每个新任务分配不同的端口值。
    mysql_connect(&pnew->conn,port);//根据自增的端口号建立 MySQL 数据库连接，并将连接信息保存到新节点中。
    strcpy(pnew->path,"/");//将默认路径 "/" 复制到新任务节点的路径成员中。
    pthread_mutex_lock(&pQueue->mutex);//加锁任务队列的互斥量，确保队列操作线程安全。
    //接下来的代码使用尾插法将新任务插入队列。
    if(!pQueue->pRear)//检查队列是否为空（即队尾指针为空）。
    {   //如果队列为空，将新节点同时作为队列的头和尾。
        pQueue->pFront=pnew;
        pQueue->pRear=pnew;
    }else{//如果队列不为空，将新节点插入到队尾，并更新队尾指针。
        pQueue->pRear->pnext=pnew;
        pQueue->pRear=pnew;
    }
    pQueue->queueSize++;//队列任务数自增。
    pthread_mutex_unlock(&pQueue->mutex);//解锁任务队列的互斥量，允许其他线程访问队列。
    return 0;//返回 0 表示任务入队成功。
}

//出队
int taskDequeue(taskQueue_t *pQueue,task_t **pnew)//定义任务出队函数，参数包括任务队列指针和存放出队任务节点地址的指针。
{
    if(pQueue->queueSize)//判断队列是否不为空（即任务数大于 0）。
    {
        *pnew=pQueue->pFront;//将队首任务节点地址赋值给传出参数。
        pQueue->pFront=pQueue->pFront->pnext;//更新队首指针为下一个任务节点。
        pQueue->queueSize--;//队列任务数减 1。
        if(NULL==pQueue->pFront)//当出队时，队列为空时，队尾要改变
        {
            pQueue->pRear=NULL;//如果队列为空，将队尾指针置为 NULL。
        }
    }else{
        return -1;//如果队列为空，则返回 -1 表示出队失败。
    }
    return 0;//返回 0 表示任务出队成功。
}