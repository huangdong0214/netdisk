#include "func.h"

//循环接收数据，要接多少字节，接完那么多才会返回
int recvn(int new_fd,void* pStart,int len)// 从 new_fd 读取 len 字节的数据
{
    int total=0,ret;// total 记录已接收的字节数
    char *p=(char*)pStart;// 把 pStart 转换为 char*，方便按字节操作
    while(total<len)// 进入循环，直到 total 等于 len，确保数据完整
    {
        ret=recv(new_fd,p+total,len-total,0);// 从 new_fd 读取剩余字节数的数据，存入 p+total
        ERROR_CHECK(ret,-1,"recv");
        if(0==ret)
        {
            return -1;//对端断开了，返回-1
        }
        total+=ret;// 更新已接收字节数
    }
    return 0;// 成功接收 len 字节数据，返回 0
}

//接收文件
int recvFile(int new_fd,char* file_name)// 负责从 new_fd 接收文件并存储到 file_name
{
    int fd=open(file_name,O_RDWR|O_CREAT|O_APPEND,0666);// 以读写模式打开（若无则创建），如果文件已存在append：往尾部写
    ERROR_CHECK(fd,-1,"open");// 检查 open() 是否失败
    int recvLen;// 用于存储接收的数据长度
    off_t fileSize;// 存储文件总大小
    recvn(new_fd,&recvLen,4);// 先接收文件大小信息的长度
    recvn(new_fd,&fileSize,recvLen);// 再接收文件的总大小

    struct timeval start,end;// 定义 timeval 结构体变量 start 和 end，用于存储时间信息
    gettimeofday(&start,NULL);// 记录开始接收的时间


    off_t downLoadSize=0;// 已下载的字节数
    off_t lastSize=0;// 上次打印进度的字节数
    off_t slice=fileSize/100;// 计算每 1% 的大小
    char buf[1000];// 用于存放接收的数据

    //接文件内容,接1000的整数倍
    while(fileSize-downLoadSize>=1000)// 每次接收 1000 字节，直到剩余不足 1000 字节
    {
        recvn(new_fd,buf,1000);//接recvLen长度的数据
        write(fd,buf,1000);// 写入文件
        downLoadSize+=1000;// 更新已接收的字节数
        if(downLoadSize-lastSize>slice)// 如果超过了 1% 的进度，则打印进度条
        {
            printf("%5.2lf%%\r", 100.0*downLoadSize/fileSize);
            fflush(stdout); // 刷新输出缓冲区
            lastSize = downLoadSize;// 更新上次打印进度的字节数
        }
    }
    recvn(new_fd,buf,fileSize-downLoadSize); // 接剩余的不足 1000 字节的数据
    write(fd,buf,fileSize-downLoadSize);// 写入文件
    
    printf("100.00%%\n");
    gettimeofday(&end,NULL);// 记录接收结束时间
    //统计下载总时间
    printf("use time=%ld\n",(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec);
    close(fd);//关闭文件
    return 0; // 返回 0，表示接收成功
}

//buf是不同的结构体,发数据
int send_protocal(int new_fd,int control_code,void *pdata,int send_len)// 发送带有协议头的数据
{
    protocol_t t; // 定义协议结构体
    int ret;
    t.size=send_len;// 告诉发多少数据
    t.control_code=control_code;// 设置控制码
    if(pdata)// 如果有数据
    {
        memcpy(t.buf,pdata,send_len);// pdata的数据复制数据到t.buf协议缓冲区，长度send_len
    }
    ret=send(new_fd,&t,8+t.size,0);// 发送协议头（8字节）+ 数据
    ERROR_CHECK(ret,-1,"send");//检查 send 是否失败
    return 0;// 发送成功
}
//buf里可以是任何数据，接数据
int recv_protocal(int new_fd,int *control_code,void *pdata)// 接收协议数据
{
    int recvLen;// 存储接收到的数据长度
    recvn(new_fd,&recvLen,4);// 先接收数据长度
    recvn(new_fd,control_code,4);// 再接收控制码
    recvn(new_fd,pdata,recvLen);// 最后接收实际数据
    return 0;// 返回 0，表示接收成功
}

//发送文件
int transFile(int new_fd,char* file_name) // 负责发送文件到 new_fd
{
    train_t t;// 定义文件传输结构体

    //打开文件
    int fd=open(file_name,O_RDONLY);// 以只读方式打开文件
    ERROR_CHECK(fd,-1,"open");// 检查 open() 是否失败

    //发送文件大小给对端
    struct stat file_buf;// 定义 stat 结构体变量 file_buf，用于存储文件属性信息
    fstat(fd,&file_buf);// 获取文件信息
    t.size=sizeof(file_buf.st_size);// 设置数据大小
    memcpy(t.buf,&file_buf.st_size,sizeof(file_buf.st_size));// 复制文件大小信息
    send(new_fd,&t,4+t.size,0);// 发送文件大小

    //通过sendfile发送文件内容
    long ret=sendfile(new_fd,fd,NULL,file_buf.st_size);// 通过 sendfile 发送文件数据
    ERROR_CHECK(ret,-1,"sendfile");// 检查 sendfile 是否失败
end:
    close(fd);// 关闭文件
    return 0;// 发送成功
}