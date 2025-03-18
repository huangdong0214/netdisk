#include "factory.h"

int factoryInit(factory_t *pFactory,int threadNum)//定义工厂初始化函数，参数为工厂结构体指针和线程数
{
    int ret;
    bzero(pFactory,sizeof(factory_t));//将工厂结构体内存清零
    pFactory->thidArr=(pthread_t*)calloc(threadNum,sizeof(pthread_t)); //为线程ID数组分配内存
    pFactory->threadNum=threadNum;//线程数目初始化
    ret=pthread_cond_init(&pFactory->cond,NULL);//条件变量初始化
    THREAD_ERR_CHECK(ret,"pthread_cond_init");
    pthread_mutex_init(&pFactory->taskQueue.mutex,NULL);//锁初始化
    // pFactory->exitFlag=0;
    return 0;//返回0表示初始化成功
}

//启动子线程
int factoryStart(factory_t *pFactory)
{
    int i,ret;
    for(i=0;i<pFactory->threadNum;i++)//循环创建子线程
    {
        ret=pthread_create(pFactory->thidArr+i,NULL,threadFunc,pFactory);//创建子线程
        THREAD_ERR_CHECK(ret,"pthread_create");//检查线程创建是否成功
    }
}


//监控某个注册描述符是否可读
int epoll_add(int epfd,int fd)//定义epoll注册函数
{
    struct epoll_event event;//定义epoll事件结构体
    event.data.fd=fd;//将目标描述符存储到epoll事件中
    event.events=EPOLLIN;//设置监听的事件类型为可读事件
    int ret=epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);//将描述符添加到epoll实例进行监控
    ERROR_CHECK(ret,-1,"epoll_ctl");
    return 0;//返回0表示注册成功
}


//解除监控某个注册描述符
int epoll_del(int epfd,int fd)//定义epoll删除函数
{
    struct epoll_event event;//定义epoll事件结构体
    event.data.fd=fd;//将目标描述符存储到epoll事件中
    event.events=EPOLLIN;//设置监听的事件类型为可读事件
    int ret=epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&event);//从epoll实例中移除目标描述符
    ERROR_CHECK(ret,-1,"epoll_ctl");
    return 0;//返回0表示解除监控成功
}


void modify_path_slash(char *path)//定义函数，用于修改路径字符串中的斜杠
{
    int len = strlen(path);//获取路径字符串的长度

    // 寻找倒数第二个斜杠的位置
    int second_last_slash ;//定义变量存储倒数第二个斜杠的位置
    for (int i = len-2; i >=0; i--) {//从倒数第二个字符开始向前遍历
        if(path[i]=='/')//判断是否是斜杠
        {
            second_last_slash=i;//记录倒数第二个斜杠位置
            break;//跳出循环
        }
    }
    path[second_last_slash+1]=0;//在倒数第二个斜杠后一位插入字符串结束符，实现路径截断
}
