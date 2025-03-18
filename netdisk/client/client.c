#include "func.h"
//这里包含了 func.h 这个头文件，说明程序的某些函数和数据结构是在 func.h 里定义的。
int main(int argc, char *argv[])
{
    ARGS_CHECK(argc, 3);//ARGS_CHECK 是一个宏或函数，检查命令行参数是否正确（至少 3 个，否则报错）。
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);//创建一个TCP连接的客户端套接字
    ERROR_CHECK(client_fd, -1, "socket");//检查 socket 是否创建成功。
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;//指定使用 IPv4
    serAddr.sin_addr.s_addr = inet_addr(argv[1]);//把 IP 地址转换为二进制格式。
    serAddr.sin_port = htons(atoi(argv[2]));//把端口号转换成网络字节序。
    int ret;
    ret = connect(client_fd, (struct sockaddr *)&serAddr, sizeof(serAddr));//connect 让客户端尝试连接服务器。
    ERROR_CHECK(ret, -1, "connect");//失败时 ERROR_CHECK 可能会报错并退出。

    // 选择注册或登录
    int control_code;
start:
    printf("1:注册,2:登录\n");
    scanf("%d", &control_code);//control_code 存储用户输入的指令（1 代表注册，2 代表登录）。
    username_passwd_t user_passwd;//username_passwd_t 是一个结构体，存储用户名和密码。
    // 注册
    if (1 == control_code)
    {
        input_username_passwd(&user_passwd);//让用户输入用户名和密码。
        send_protocal(client_fd, control_code, &user_passwd, sizeof(user_passwd));//发送注册请求。
        recv_protocal(client_fd, &control_code, NULL);//接收服务器的响应，判断注册是否成功。
        if (control_code == REGISTER_FAILED)
        {
            printf("该账号已注册，请直接登录。\n");
            goto start;//如果失败，goto start 让用户重新选择。
        }
        printf("注册成功，已登录\n");
    }
    // 登录
    else if (2 == control_code) 
    {
        input_username_passwd(&user_passwd);//让用户输入用户名和密码。
        send_protocal(client_fd, control_code, &user_passwd, sizeof(user_passwd));//发送登录请求。
        // printf("发送成功\n");
        recv_protocal(client_fd, &control_code, NULL);//接收服务器的响应，判断登录是否成功。
        if (control_code == LOGIN_FAILED)
        {
            goto start;//如果失败，goto start 让用户重新选择。
        }
        printf("登录成功\n");
    }
    else
    {
        printf("错误输入\n");
        goto start;
    }
    getchar(); // 去除换行
    // 登录成功,开始发各种命令
    char cmd[1000];//cmd用于存储用户输入的命令。
    char path[1000]; //存储各个命令后面带的参数
    while (1)
    {
        fgets(cmd, sizeof(cmd), stdin); //fgets 读取用户输入的命令。
        printf("cmd=%s\n", cmd);
        cmd[strlen(cmd) - 1] = 0; //去除换行符\n
        system("clear");//清屏。
        if (strncmp("cd", cmd, 2) == 0)// 如果用户输入 "cd" 命令
        {
            // cmd+3就是cd后面带的文件夹名字
            send_protocal(client_fd, CD, cmd + 3, strlen(cmd) - 3);//发送cd 目标目录到服务器
            bzero(path, sizeof(path));//清空 path 数组，防止残留数据
            recv_protocal(client_fd, &control_code, path); // 接收服务器返回的执行结果（是否切换成功）和新路径
            if (SUCCESS == control_code)// 判断服务器是否切换目录成功
            {
                printf("%s\n", path);// 打印新的目录路径
            }
            else
            {
                printf("文件夹不存在，切换失败\n");// 提示用户切换目录失败
            }
        }
        else if (strncmp("ls", cmd, 2) == 0)// 如果用户输入 "ls" 命令
        {
            send_protocal(client_fd, LS, NULL, 0);// 发送 "ls" 命令到服务器，请求当前目录文件列表
            int recvLen;// 用于存储服务器发送的文件名长度
            char buf[1024];// 缓冲区，用于存储接收到的文件名数据
            while (1) // 循环接收服务器返回的文件列表
            {
                recvn(client_fd, &recvLen, 4); // 先接收服务器发送的文件名长度
                if (recvLen > 0)// 如果接收到有效长度，说明还有文件名需要读取
                {
                    bzero(buf,sizeof(buf));// 清空 buf 数组，防止残留数据
                    recvn(client_fd, buf, recvLen);  // ls接收一行打印一行
                    puts(buf);    
                }
                else
                {
                    break;// 如果文件名长度为 0，说明服务器文件列表发送完毕，退出循环
                }
            }
        }
        else if (strncmp("puts", cmd, 4) == 0)// 如果用户输入 "puts" 命令
        {
            //发送控制码，文件名
            send_protocal(client_fd, PUTS, cmd + 5, strlen(cmd) - 5);// 发送 "puts" 命令和文件名到服务器
            //获取文件的md5值并发送
            char md5_str[50]={0};// 存储计算出的 MD5 字符串
            Compute_file_md5(cmd+5,md5_str);// 计算文件的 MD5
            printf("%s\n",md5_str);// 打印 MD5 值，方便调试
            train_t t;
            t.size=strlen(md5_str);// 设置要发送的数据长度
            strcpy(t.buf,md5_str);// 将 MD5 值复制到 t.buf
            send(client_fd,&t,4+t.size,0);// 发送 MD5 值给服务器
            //判断是否秒传成功
            recv_protocal(client_fd, &control_code, NULL); // 接收服务器的秒传判断结果
            if (SUCCESS == control_code)// 如果服务器返回秒传成功
            {
                printf("秒传成功\n");
            }
            else
            {
                printf("秒传失败\n");
                transFile(client_fd,cmd + 5);// 发送文件数据给服务器
                printf("发送成功\n");
            }

        }
        else if (strncmp("gets", cmd, 4) == 0)// 如果用户输入 "gets" 命令
        {
            off_t seek_pos;// 用于存储文件的续传位置（文件已经下载的大小）
            struct stat file_buf;// 用于存储本地文件的状态信息（比如大小、权限等）
            
            int fd=open(cmd+5,O_RDONLY);
            if(-1==fd)// 文件不存在
            {
                printf("文件不存在\n");// 从头开始下载
                seek_pos=0;
            }else{// 文件存在
                fstat(fd,&file_buf); // 获取文件信息
                seek_pos=file_buf.st_size;// 记录文件当前大小（续传位置）
                close(fd);// 关闭文件
            }
            char new_cmd[1200]={0}; // 存储带续传位置的新命令
            sprintf(new_cmd,"%s %ld",cmd,seek_pos);// 拼接命令和续传位置
            send_protocal(client_fd, GETS, new_cmd + 5, strlen(new_cmd) - 5);// 发送 "gets" 命令和续传位置
            recvFile(client_fd,cmd+5);// 接收文件数据并保存
        }
        else if (strncmp("rm", cmd, 2) == 0)// 如果用户输入 "rm" 命令，删除普通文件或空目录，非空目录会删除失败
        {
            send_protocal(client_fd, RM, cmd + 3, strlen(cmd) - 3);// 发送 "rm" 命令和目标文件/目录名到服务器
            recv_protocal(client_fd, &control_code, NULL); // 接收服务器返回的执行结果
            if (SUCCESS == control_code)// 服务器返回删除成功
            {
                printf("删除成功\n");
            }
            else
            {
                printf("目录非空删除失败\n");
            }
        }
        else if (strncmp("pwd", cmd, 3) == 0)// 如果用户输入 "pwd" 命令
        {
            send_protocal(client_fd, PWD, NULL, 0); // 发送 "pwd" 命令到服务器，请求当前路径
            bzero(path, sizeof(path));// 清空 path 数组，防止残留数据
            recv_protocal(client_fd, &control_code, path);// 接收服务器返回的当前路径
            printf("%s\n", path);  // 打印当前目录路径
        }
        else if (strncmp("mkdir", cmd, 5) == 0)// 如果用户输入 "mkdir" 命令
        {
            // cmd+6数据指针往后偏移6 就是mkdir 后面带的文件夹名字，
            //strlen(cmd) - 6  命令总长度-6就是目录名的长度
            send_protocal(client_fd, MKDIR, cmd + 6, strlen(cmd) - 6);// 发送 "mkdir" 命令和目录名到服务器
            recv_protocal(client_fd, &control_code, NULL); // 接收服务器返回的创建文件夹成功还是失败
            if (SUCCESS == control_code)// 服务器返回创建成功
            {
                printf("创建成功\n");
            }
            else
            {
                printf("文件夹存在，创建失败\n");
            }
        }
        else
        {
            continue;// 如果输入的命令不符合任何已知命令，直接忽略并等待下一次输入
        }
    }
    close(client_fd);// 关闭客户端与服务器的连接
    return 0;// 程序正常结束
}