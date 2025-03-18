#ifndef __HEAD_H__
#define __HEAD_H__
#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <strings.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <syslog.h>
#include <mysql/mysql.h>
#include <sys/sendfile.h>
#define ARGS_CHECK(argc,num) {if(argc!=num) {printf("error args\n");return -1;}}
#define ERROR_CHECK(ret,retval,func_name) {if(ret==retval) {perror(func_name);printf("filename=%s,line=%d\n",__FILE__,__LINE__);return -1;}}
#define THREAD_ERR_CHECK(ret,func_name) {if(ret!=0) {printf("%s failed,%d %s\n",func_name,ret,strerror(ret));return -1;}}

//这个结构体两个成员位置不可以交换
typedef struct{
    int size;//火车头，代表火车车厢上放了多少数据
    char buf[1000];//任何数据都可以装
} train_t;


//协议火车
typedef struct{
    int size;//火车大小
    int control_code;//控制码
    char buf[1000];
}protocol_t;


//用户名和密码存储的结构体
typedef struct
{
    char user_name[20];
    char passwd[50];
}username_passwd_t;


enum CONTROL{
    REGISTER,
    LOGIN,
    LS,
    CD,
    PWD,
    MKDIR,
    RM,
    GETS,
    PUTS,
    REGISTER_SUCCESS=100,//这里开始是客户端的控制码
    REGISTER_FAILED,
    LOGIN_SUCCESS,
    LOGIN_FAILED,
    SUCCESS,
};


int recvFile(int new_fd,char* file_name);

int input_username_passwd(username_passwd_t*);

//buf是不同的结构体,发数据
int send_protocal(int new_fd,int control_code,void *pdata,int send_len);

//buf里可以是任何数据，接数据
int recv_protocal(int new_fd,int *control_code,void *pdata);

int recvn(int new_fd,void* pStart,int len);

//发送文件
int transFile(int new_fd,char* file_name);

int Compute_file_md5(char *file_path, char *md5_str);
#endif
