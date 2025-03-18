#include "factory.h"

//管道和标记变量
int exit_pipeFd[2];
int exit_flag=0;
//退出信号处理函数 exit_sigfunc
void exit_sigfunc(int signum)
{
    printf("要有序退出了\n");
    write(exit_pipeFd[1],"1",1);//向预先定义的管道（exit_pipeFd[1]）写入字符 "1"，用于通知主进程或其他线程需要退出；
    exit_flag=1;//将全局退出标志（exit_flag）设置为 1，从而在后续程序逻辑中触发有序退出。
}

int main(int argc,char* argv[])
{
    if(argc!=4)// 检查命令行参数是否正确，正确格式应为：./netdisk_server IP PORT THREAD_NUM
    {
        printf("./netdisk_server IP PORT THREAD_NUM\n");
        return -1;
    }
    //有序退出
    pipe(exit_pipeFd);// 创建管道，用于进程间通信，控制服务器的有序退出
    while(fork())// 创建子进程，并监控子进程状态
    {
        signal(SIGUSR1,exit_sigfunc);// 设置信号处理函数，捕捉 SIGUSR1 信号
        wait(NULL);// 等待子进程结束
        if(exit_flag)//如果退出标志被设置
        {
            exit(0);// 退出父进程
        }
        printf("子进程挂了，或走到异常分支啦，重启它\n");// 子进程异常终止时，重新启动子进程
    }
    //子进程走到下面去

    int threadNum=atoi(argv[3]);// 获取线程池线程数
    printf("threadNum=%d\n",threadNum);// 打印线程池线程数
    factory_t f;// 定义线程池工厂结构体
    factoryInit(&f,threadNum); // 初始化工厂
    factoryStart(&f);// 启动线程池，创建指定数量的线程
    int sock_fd,new_fd; // 定义服务器监听 socket 和客户端连接 socket
    
    tcp_init(argv[1],argv[2],&sock_fd);// 初始化服务器监听 socket，绑定 IP 和端口

    struct sockaddr_in clientAddr;// 定义客户端地址结构体
    socklen_t addrLen;// 定义地址长度变量

    taskQueue_t *pQue=&f.taskQueue;// 获取线程池的任务队列指针
    //注册监控sock_fd
    int epfd=epoll_create(1);// 创建 epoll 句柄，用于 I/O 事件监听
    epoll_add(epfd,sock_fd);// 将服务器监听 socket 添加到 epoll 监听列表


    //注册监控退出管道的读端
    epoll_add(epfd,exit_pipeFd[0]);// 将退出管道的读端添加到 epoll 监听列表

    int ready_fd_num,i,j,epoll_ctl_num=2;// epoll 监听的文件描述符数量，初始为 2（监听 socket 和退出管道）
    struct epoll_event evs[100];// 定义 epoll 事件数组，支持最多 100 个用户并发连接
    MYSQL *conn;// 定义 MySQL连接指针
    mysql_connect(&conn,0); // 主线程连接数据库
    while(1)
    {
        ready_fd_num=epoll_wait(epfd,evs,epoll_ctl_num,-1);// 等待 epoll 事件触发
        //服务器使用 epoll_wait 监听所有已注册的文件描述符sock_fd。
        //-1：表示 epoll_wait 一直等待，直到有事件发生。
        for(i=0;i<ready_fd_num;i++)// 遍历所有发生事件的文件描述符
        {
            //处理epoll返回的事件
            if(evs[i].data.fd==sock_fd)// 监听 socket 事件（有新的客户端连接）
            {
                addrLen=sizeof(clientAddr);
                new_fd=accept(sock_fd,(struct sockaddr*)&clientAddr,&addrLen);
                //接受新的客户端连接，返回新的 new_fd（客户端的socket文件描述符）。
                ERROR_CHECK(new_fd,-1,"accept");
                epoll_add(epfd,new_fd);// 将新的客户端连接 socket 添加到 epoll 监听列表
                epoll_ctl_num+=1;// 增加 epoll 监听的文件描述符数量
            }
            //处理退出信号
            else if(evs[i].data.fd==exit_pipeFd[0]) // 监听退出信号
            {
                printf("线程池开始退出\n");
                pthread_mutex_lock(&pQue->mutex);
                f.exitFlag=1;//设置退出标志，让线程池中的线程都知道需要退出。
                pthread_mutex_unlock(&pQue->mutex);
                for(j=0;j<threadNum;j++)
                {
                    pthread_cond_signal(&f.cond);//唤醒所有线程，防止线程池中的线程一直阻塞在 wait 状态。
                }
                for(j=0;j<threadNum;j++)
                {
                    pthread_join(f.thidArr[j],NULL);// 等待所有线程退出完毕。
                }
                printf("线程池退出成功\n");
                exit(0);//服务器进程退出。
            }
            //处理客户端请求
            else{
                int control_code;//操作类型（1：注册，2：登录）。
                uint64_t login_flag;//用于存储登录成功与否的标志。
                username_passwd_t user_passwd;//存储 用户名和密码 的结构体。
                int user_id;
                recv_protocal(evs[i].data.fd,&control_code,&user_passwd);// 服务器接收客户端发来的用户名密码
                //从 evs[i].data.fd 读取客户端发送的数据，解析出 请求类型（注册/登录） 和 用户名/密码。
#ifdef DEBUG
                printf("%s %s\n",user_passwd.user_name,user_passwd.passwd);
#endif
                //处理注册请求
                if(1==control_code)
                {
                    //调用 check_username_exist() 查询数据库，检查用户名是否已存在
                    if(check_username_exist(conn,user_passwd.user_name))
                    {
                        send_protocal(evs[i].data.fd,REGISTER_FAILED,NULL,0);//如果已存在：发送注册失败给客户端。
                    }
                    else{
                        //向数据库插入新用户，把对应的用户名和密码放入数据库
                        login_flag=insert_new_user(conn,&user_passwd);
                        send_protocal(evs[i].data.fd,REGISTER_SUCCESS,NULL,0);// 发送注册成功消息
                    }
                }
                //处理登录请求
                else if(2==control_code)
                {
                    login_flag=check_login(conn,&user_passwd);
                    //验证用户名和密码：调用 check_login() 检查数据库中的用户信息。
                    if(login_flag)
                    {
                        send_protocal(evs[i].data.fd,LOGIN_SUCCESS,NULL,0);// 给客户端发送登录成功
                    }else{
                        send_protocal(evs[i].data.fd,LOGIN_FAILED,NULL,0);// 给客户端发送登录失败
                    }
                }
                //登录成功后，服务器处理该用户的后续操作（如文件上传/下载）。
                if(login_flag)
                {
                    printf("主线程放任务到队列\n");
                    user_id=get_user_id(conn,user_passwd.user_name);// 获取用户 ID
                    printf("%d\n",user_id);
                    taskEnQueue(pQue,evs[i].data.fd,user_id);//将任务放入队列，交给子线程处理。
                    epoll_del(epfd,evs[i].data.fd);//从 epoll 监听列表中移除该客户端。
                    epoll_ctl_num-=1;//监控的描述符数目减一
                    pthread_cond_signal(&f.cond);//唤醒一个子线程处理任务
                }
            }
        }

     
    }
    close(sock_fd);//关闭服务器
    return 0;// 退出程序
}