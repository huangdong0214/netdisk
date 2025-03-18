// 子线程函数放这里
#include "factory.h"

void cleanup(void *p)// 线程清理函数
{
    pthread_mutex_t *pmutex = (pthread_mutex_t *)p;// 将 void 指针转换为互斥锁指针
    pthread_mutex_unlock(pmutex); // 解锁互斥锁
}

// 子线程流程
void *threadFunc(void *p)// 线程函数，处理任务队列中的任务
{
    factory_t *pFactory = (factory_t *)p;// 将参数转换为工厂指针
    taskQueue_t *pQue = &pFactory->taskQueue;// 获取任务队列指针
    task_t *pnew;
    int taskFlag; 
    while (1)
    {
        pthread_mutex_lock(&pQue->mutex);            // 加锁
        pthread_cleanup_push(cleanup, &pQue->mutex); // 注册清理函数，确保退出时解锁
        if (!pQue->queueSize)                        // 判断任务队列是否为空
        {
            pthread_cond_wait(&pFactory->cond, &pQue->mutex); // 任务队列为空时阻塞等待
        }
        if (pFactory->exitFlag)// 判断是否需要退出
        {
            printf("子线程退出\n");
            pthread_exit(NULL);// 退出线程
        }
        // 出队
        taskFlag = taskDequeue(pQue, &pnew);// 任务出队
        pthread_cleanup_pop(1); // 解除清理函数，并解锁
        if (!taskFlag)          // 如果成功获取到任务
        {
            printf("开始处理客户端的各种请求\n");// 打印处理任务信息
            client_handle(pnew); // 处理客户端请求
            mysql_close(pnew->conn);// 关闭数据库连接
            close(pnew->new_fd); // 关闭客户端 socket
            free(pnew); // 释放任务结构体内存
        }
        if (pFactory->exitFlag)// 再次检查是否需要退出
        {
            printf("子线程退出\n"); // 打印退出信息
            pthread_exit(NULL);// 退出线程
        }
    }
}

void client_handle(task_t *pnew)// 处理客户端请求
{
    int control_code;// 操作码
    int new_fd = pnew->new_fd;// 获取客户端 socket
    int ret;
    char client_data[2000];// 存储客户端发送的数据

    while (1)//无限循环处理客户端请求
    {
        bzero(client_data,sizeof(client_data));// 清空数据缓冲区
        ret=recv_protocal(new_fd, &control_code, client_data);// 接收客户端请求
        printf("control_code %d\n",control_code);// 打印操作码
        if(-1==ret)// 判断客户端是否断开
        {
            printf("客户端断开\n");
            break;// 退出循环
        }
        switch (control_code)// 根据不同的操作码执行不同的功能
        {
        case LS:
            do_ls(pnew);
            break;
        case CD:
            do_cd(pnew,client_data);
            break;
        case PWD:
            do_pwd(pnew);
            break;
        case MKDIR:
            do_mkdir(pnew,pnew->conn,client_data);
            break;
        case RM:
            do_rm(pnew,client_data);
            break;
        case GETS:
            do_gets(pnew,client_data);
            break;
        case PUTS:
            do_puts(pnew,client_data);
            break;
        default:
            break;
        }
    }
}

void do_ls(task_t *pnew)// 处理 ls 命令
{
    send_file_info(pnew);// 发送文件信息给客户端
}
// cd  ..
// cd 目录名 ;path参数存储的是..或者目录名
void do_cd(task_t *pnew,char *path)// 处理 cd 命令
{
    //如果是..
    if(!strcmp(path,".."))// 判断是否是返回上级目录
    {
        if(0!=pnew->path_id)// 如果当前不在根目录
        {
            //通过pnew->path_id得到pre_id
            pnew->path_id=get_pre_id(pnew->conn,pnew->path_id); // 获取上级目录 ID
            //pnew->path
            modify_path_slash(pnew->path);// 更新路径
            LOG(pnew->path);//打印字符串
        }
        send_protocal(pnew->new_fd,SUCCESS,pnew->path,strlen(pnew->path));
    }
    else// 进入指定目录
    {
        int path_id=check_cd_dir_isexist(pnew->conn,pnew->user_id,pnew->path_id,path);// 检查目录是否存在
        if(path_id)
        {
            //改变path，和path_id;
            char new_path[2000]={0};// 定义新路径
            sprintf(new_path,"%s%s/",pnew->path,path);// 生成新路径
            strcpy(pnew->path,new_path);// 更新路径
            pnew->path_id=path_id;// 更新路径 ID
            send_protocal(pnew->new_fd,SUCCESS,pnew->path,strlen(pnew->path));// 切换成功
        }
        else{
            send_protocal(pnew->new_fd,FAILED,NULL,0);//切换失败
        }
    }
}
void do_pwd(task_t *pnew)// 处理 pwd 命令
{
    send_protocal(pnew->new_fd,SUCCESS,pnew->path,strlen(pnew->path));// 发送当前路径
}

void do_mkdir(task_t *pnew,MYSQL* conn,char* dir_name)// 处理 mkdir 命令
{
    int dir_isexist;
    dir_isexist=check_dir_isexist(conn,pnew->user_id,pnew->path_id,dir_name);// 检查目录是否存在
    printf("dir_isexist %d\n",dir_isexist);
    if(dir_isexist)//存在同名文件，不能创建
    {
        send_protocal(pnew->new_fd,FAILED,NULL,0);// 目录已存在，发送失败响应
    }
    else{
        creat_dir(conn,pnew->user_id,pnew->path_id,dir_name);// 创建目录
        send_protocal(pnew->new_fd,SUCCESS,NULL,0);// 创建成功
    }
}
void do_rm(task_t *pnew,char* rm_file_name)// 处理 rm 命令
{
    int rm_flag;
    rm_flag=rm_file_emptydir(pnew,rm_file_name);// 调用 rm_file_emptydir 函数删除文件或空目录
    if(rm_flag) // 判断删除是否成功
    {
        send_protocal(pnew->new_fd,SUCCESS,NULL,0);// 发送删除成功给客户端
    }
    else{
        send_protocal(pnew->new_fd,FAILED,NULL,0);// 发送删除失败给客户端
    }
}
void do_gets(task_t *pnew,char *file_name_and_len)// 处理 gets 命令
{
    off_t seek_pos,file_size;// 定义变量 seek_pos 存储文件偏移量，file_size 存储文件总大小
    seek_pos=get_file_name_pos(file_name_and_len);// 获取文件偏移量
    char md5_str[50]={0};// 定义并初始化 md5 字符串数组
    //获得文件md5和大小
    get_md5_and_file_size(pnew,md5_str,&file_size,file_name_and_len); // 获取文件 MD5 和大小
    //发送文件,md5值就是文件名,file_size-seek_pos就是实际要发送的大小
    transFile(pnew->new_fd,md5_str,seek_pos,file_size-seek_pos);// 传输文件
}
void do_puts(task_t *pnew,char* file_name)// 处理 puts 命令
{
    char md5_str[50]={0};
    int recvLen;
    recvn(pnew->new_fd,&recvLen,4);//接火车头，接收表示数据长度的 4 字节数据
    recvn(pnew->new_fd,md5_str,recvLen);// 接收 md5 字符串数据，长度为 recvLen
    uint64_t second_transfer;
    second_transfer=check_file_md5_exist(pnew,md5_str);// 检查文件 md5 是否已存在，实现秒传
    if(second_transfer)// 如果秒传成功
    {
        //在file_system表中插入一条记录，给客户端回复上传成功
        creat_file(pnew->conn,pnew->user_id,pnew->path_id,file_name,md5_str);// 创建文件记录
        send_protocal(pnew->new_fd,SUCCESS,NULL,0);// 发送成功协议给客户端
    }
    else{
        send_protocal(pnew->new_fd,FAILED,NULL,0);// 发送秒传失败给客户端，客户端收到秒传失败传文件给服务器端
        off_t file_size;
        file_size=recvFile(pnew->new_fd,md5_str);// 接收文件
        //在file_md5sum新增一条记录
        creat_file_md5(pnew->conn,md5_str,file_size);// 创建文件 md5 
        //在file_system表中插入一条记录
        creat_file(pnew->conn,pnew->user_id,pnew->path_id,file_name,md5_str);
        printf("上传成功\n");
    }
}