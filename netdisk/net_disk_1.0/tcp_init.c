#include "head.h"

//socket,bind,listen的初始化
int tcp_init(char* ip,char* port,int *sfd)//定义初始化 TCP 连接的函数，参数分别为 IP 地址、端口和存放套接字描述符的指针。
{
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);//创建一个 TCP 套接字，并返回套接字描述符。
    ERROR_CHECK(sock_fd,-1,"socket");
    struct sockaddr_in serAddr;//定义存储服务器地址信息的结构体变量。
    serAddr.sin_family=AF_INET;//设置地址族为 IPv4。
    serAddr.sin_addr.s_addr=inet_addr(ip);//将 IP 地址字符串转换为网络字节序的整数，存储在结构体中。
    serAddr.sin_port=htons(atoi(port));//将端口字符串转换为整数，再转换为网络字节序存储在结构体中。
    int ret;

    //地址重用
    int reuse=1;//定义变量 reuse 并赋值 1，表示启用地址重用。
    ret=setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));//调用 setsockopt 设置套接字选项，允许地址重用。
    ERROR_CHECK(ret,-1,"setsockopt");//检查 setsockopt 是否成功，若失败则输出错误信息。

    ret=bind(sock_fd,(struct sockaddr*)&serAddr,sizeof(serAddr));//将套接字绑定到指定的 IP 地址和端口上。
    ERROR_CHECK(ret,-1,"bind");
    listen(sock_fd,10);//使套接字进入监听状态，最多允许 10 个等待连接的客户端。
    *sfd=sock_fd;//将创建的套接字描述符赋值给传入的指针，便于函数外部使用。
    return 0;
}