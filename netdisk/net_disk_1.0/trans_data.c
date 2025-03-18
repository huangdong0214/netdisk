#include "factory.h"

//发送文件
int transFile(int new_fd,char* md5_file_name,off_t seek_pos,off_t remaining_file_size)
{
    char path_file_name[1000]="net_root/";// 定义文件路径字符串，初始化为 "net_root/"
    
    strcat(path_file_name,md5_file_name);//拼接文件所在路径
    int fd=open(path_file_name,O_RDONLY);
    ERROR_CHECK(fd,-1,"open");

    train_t t; // 定义传输结构体
    //发送 remaining_file_size给客户端
    t.size=sizeof(remaining_file_size);// 设置数据包大小
    memcpy(t.buf,&remaining_file_size,sizeof(remaining_file_size));// 复制文件大小到数据包
    send(new_fd,&t,4+t.size,0);// 发送文件大小信息给客户端

    //通过sendfile发送文件内容，从seek_pos开始发
    long ret;
    if(seek_pos)// 如果指定了起始偏移量
    {
        ret=sendfile(new_fd,fd,&seek_pos,remaining_file_size);// 从指定偏移量开始发送文件
        ERROR_CHECK(ret,-1,"sendfile");// 检查发送是否成功
    }
    else{
        ret=sendfile(new_fd,fd,NULL,remaining_file_size); // 从头开始发送文件
        ERROR_CHECK(ret,-1,"sendfile");
    }

end:
    close(fd);// 关闭文件
    return 0;// 返回 0，表示发送成功
}

//循环接收，要接多少字节，接完那么多才会返回
int recvn(int new_fd,void* pStart,int len)// 接收指定长度的数据
{
    int total=0,ret;// 定义变量，total 记录已接收字节数，ret 存储 recv 返回值
    char *p=(char*)pStart;// 将 void* 转换为 char*，方便指针运算
    while(total<len)// 当接收的数据小于目标长度时，持续接收
    {
        ret=recv(new_fd,p+total,len-total,0);// 接收数据，并存储到缓冲区
        ERROR_CHECK(ret,-1,"recv");
        if(0==ret)// 如果 recv 返回 0，表示对方已断开连接
        {
            printf("对方断开了\n");
            return -1;//对端断开了，返回-1
        }
        total+=ret;// 更新已接收字节数
    }
    return 0;// 返回 0，表示接收成功
}

// //接收文件
// int recvFile(int new_fd)
// {
//     //先接文件名,每次先接4个字节
//     int recvLen;
//     char buf[1000]={0};
//     recvn(new_fd,&recvLen,4);
//     recvn(new_fd,buf,recvLen);
//     printf("buf=%s\n",buf);
//     int fd=open(buf,O_WRONLY|O_CREAT,0666);
//     ERROR_CHECK(fd,-1,"open");

//     //再接文件内容
//     while(1)
//     {
//         recvn(new_fd,&recvLen,4);//接火车车厢
//         if(recvLen>0)
//         {
//             recvn(new_fd,buf,recvLen);//接recvLen长度的数据
//             write(fd,buf,recvLen);//把接收到的内容写入文件
//         }else{
//             break;
//         }
//     }
//     close(fd);//关闭文件
//     return 0;
// }

//buf是不同的结构体,发数据
int send_protocal(int new_fd,int control_code,void *pdata,int send_len)// 发送协议数据
{
    protocol_t t;// 定义协议结构体
    int ret;
    t.size=send_len;// 设置数据大小
    t.control_code=control_code;// 设置控制码
    if(pdata)// 如果数据指针不为空
    {
        memcpy(t.buf,pdata,send_len);// 复制数据到协议结构体
    }
    ret=send(new_fd,&t,8+t.size,0);// 发送协议数据
    ERROR_CHECK(ret,-1,"send");
    return 0;// 返回 0，表示发送成功
}
//buf里可以是任何数据，接数据
int recv_protocal(int new_fd,int *control_code,void *pdata)// 接收协议数据
{
    int recvLen;// 存储接收到的数据长度
    int ret;
    ret=recvn(new_fd,&recvLen,4);// 先接收数据长度
    if(-1==ret)// 如果接收失败
    {
        return -1;
    }
    ret=recvn(new_fd,control_code,4);// 再接收控制码
    if(-1==ret)
    {
        return -1;
    }
    ret=recvn(new_fd,pdata,recvLen);// 最后接收实际数据
    if(-1==ret)
    {
        return -1;
    }
    return 0;// 返回 0，表示接收成功
}

//接收文件
off_t recvFile(int new_fd,char *md5_file_name)
{
    //先接文件名,每次先接4个字节
    int recvLen;// 存储接收到的数据长度
    char path_file_name[1000]="net_root/";// 定义存储路径字符串
    int ret;

    strcat(path_file_name,md5_file_name);// 拼接完整文件路径
    int fd=open(path_file_name,O_RDWR|O_CREAT,0666);// 以可读写模式打开或创建文件
    ERROR_CHECK(fd,-1,"open");

    //接文件大小
    off_t fileSize;// 存储文件总大小
    recvn(new_fd,&recvLen,4);// 接收文件大小信息的长度
    recvn(new_fd,&fileSize,recvLen);// 接收文件大小

    struct timeval start,end;// 定义时间变量
    gettimeofday(&start,NULL);// 获取开始时间

    off_t downLoadSize=0;// 记录下载的大小
    off_t lastSize=0;
    off_t slice=fileSize/100;// 计算 1% 的数据大小

    int pipefds[2];// 定义管道
    pipe(pipefds);//初始化管道
    int total = 0;// 记录已接收数据总量
    while(total < fileSize){// 当未接收完所有数据时，循环接收
        //从new_fd读取4096字节数据，写入管道,ret是代表写入管道的数据量
        int ret =splice(new_fd,NULL,pipefds[1],NULL,4096,SPLICE_F_MORE);// 从 socket 读取数据到管道
        if(ret<=0)
        {
            printf("对端断开了\n");
            break;// 退出循环
        }
        total += ret; // 更新已接收数据总量
        if(total-lastSize>slice)// 每接收 1% 数据，更新进度
        {
            printf("%5.2lf%%\r", 100.0*total/fileSize);// 打印进度
            fflush(stdout);// 刷新标准输出
            lastSize = total;// 更新上次记录的进度
        }
        //从管道中读取数据，写入到文件，写ret个字节
        ret=splice(pipefds[0],NULL,fd,NULL,ret,SPLICE_F_MORE);// 从管道读取数据写入文件
        if(ret<=0)// 如果写入失败
        {
            printf("对端断开了\n");
            break;// 退出循环
        }
    }
    printf("100.00%%\n");// 打印下载完成
    gettimeofday(&end,NULL);// 获取结束时间
    //统计下载总时间
    printf("use time=%ld\n",(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec);// 计算并打印总耗时
    close(fd);//关闭文件
    return fileSize;// 返回文件大小
}