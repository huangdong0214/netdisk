#ifndef __FACTORY_H__
#define __FACTORY_H__
#include "head.h"
#include "taskQueue.h"

typedef struct{
    pthread_t *thidArr;//存线程id的起始地址
    int threadNum;//线程数目
    pthread_cond_t cond;//条件变量
    taskQueue_t taskQueue;//队列
    int exitFlag;//0代表运行，1代表要退出
}factory_t;


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
    REGISTER_SUCCESS=100,//这里开始是客户端的控制码，注册成功
    REGISTER_FAILED,//注册失败
    LOGIN_SUCCESS,
    LOGIN_FAILED,
    SUCCESS,
    FAILED,
};

//工厂初始化
int factoryInit(factory_t *pFactory, int threadNum);
//工厂启动
int factoryStart(factory_t *pFactory);

//子线程函数
void * threadFunc(void* p);

//socket,bind,listen的初始化
int tcp_init(char* ip,char* port,int *sfd);

//发送文件
int transFile(int new_fd,char* file_name,off_t seek_pos,off_t remaining_file_size);

//注册监控某个描述符是否可读
int epoll_add(int epfd,int fd);


//buf是不同的结构体,发数据
int send_protocal(int new_fd,int control_code,void *pdata,int send_len);

//buf里可以是任何数据，接数据
int recv_protocal(int new_fd,int *control_code,void *pdata);

int split_user_passwd(char* usrname_passwd,char* username,char* passwd);

//为每个子线程初始化一个连接
int mysql_connect(MYSQL **conn,int port);

int check_username_exist(MYSQL *conn,char *username);

//把用户名和密码插入数据库
uint64_t insert_new_user(MYSQL *conn,username_passwd_t *p);


void generate_random_string(char *random_string);


int check_login(MYSQL *conn,username_passwd_t *p);

int get_user_id(MYSQL *conn,char *username);

int epoll_del(int epfd,int fd);


void client_handle(task_t *pnew);//处理客户端ls,cd等

void do_ls(task_t* pnew);

void do_cd(task_t *pnew,char *path);

void do_pwd(task_t* pnew);

void do_mkdir(task_t* pnew,MYSQL* conn,char*);

void do_rm(task_t *pnew,char* rm_file_name);

void do_gets(task_t *pnew,char *file_name_and_len);

void do_puts(task_t *pnew,char* file_name);

int check_dir_isexist(MYSQL* conn,int user_id,int path_id,char* dir_name);

uint64_t creat_dir(MYSQL* conn,int user_id,int path_id,char* dir_name);

int check_cd_dir_isexist(MYSQL* conn,int user_id,int path_id,char* dir_name);

int get_pre_id(MYSQL* conn,int path_id);

void modify_path_slash(char *path);

void send_file_info(task_t* pnew);

int rm_file_emptydir(task_t *pnew,char* rm_file_name);

//循环接收，要接多少字节，接完那么多才会返回
int recvn(int new_fd,void* pStart,int len);

uint64_t  check_file_md5_exist(task_t* pnew,char* md5_str);

uint64_t creat_file(MYSQL* conn,int user_id,int path_id,char* file_name,char* md5_str);

//接收文件
off_t recvFile(int new_fd,char *md5_file_name);

uint64_t creat_file_md5(MYSQL* conn,char* md5_str,off_t file_size);

off_t get_file_name_pos(char *file_name_and_len);

void get_md5_and_file_size(task_t* pnew,char* md5_str,off_t* file_size,char* file_name);
#endif