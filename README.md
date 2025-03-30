# netdisk
虚拟文件服务器（功能类似百度网盘）
main 函数
主线程启动线程池(循环调用pthread_create接口)
初始化服务器监听 Socket，绑定指定 IP 和端口号(调用bind接口)并调用listen使服务器处于监听状态；
创建 epoll 文件描述符，并调用epoll_ctl将服务器监听 Socket 添加到 epoll 监听列表。
主线程连接数据库
服务器进入循环，调用 epoll_wait 接口等待事件发生。
如果服务器监听 Socket 有事件发生，调用 accept 接口接收新的客户端连接，调用epoll_ctl将其添加到 epoll 监听列表。
否则处理客户端注册或登录
登录成功后，将任务放入任务队列，交给线程池处理。然后调用epoll_ctl从 epoll 监听列表中移除该客户端，唤醒一个子线程处理任务

客户端：
调用socket接口创建一个 TCP 连接的客户端套接字，并调用 connect 接口与服务器建立连接。
客户端启动后，提示用户选择1.注册 2.登录，并输入用户名和密码。
客户端将注册或登录的控制码及用户名和密码通过send_protocal协议结构体发送到服务器。
服务器端通过 recv_protocal 协议结构体读取数据，解析出控制码和结构体中的用户名和密码。
如果控制码为1即注册： 
服务器检查数据库中是否已有该用户名。如果用户名已存在，服务器通过 send_protocal 给客户端发送注册失败。
如果用户名不存在，将用户名及使用SHA-512加密算法加密的密码($6$拼接长度为16的随机字符串并调用crypt_r生成密文）存入用户表并给客户端发送注册成功。
如果控制码为 2即登录： 
服务器检查提供的用户名以及使用SHA-512加密算法得到的加密密码是否与数据库的一致。
如果一致，给客户端发送登录成功，否则发送登录失败。
客户端通过 recv_protocal 接收服务器返回的响应。如果失败，go to 1.注册 
2.登录
客户端进入循环，调用fgets读取并用strncmp对比用户输入的命令
pwd查看当前目录路径：
客户端调用 send_protocal(client_fd, PWD, NULL, 0)，将PWD控制码发送给服务器。
子线程调用recv_protocal(new_fd, &control_code, client_data)接收客户端请求，switch根据不同的控制码（枚举变量）执行不同的命令处理函数
此时执行PWD处理函数，调用send_protocal(pnew->new_fd,SUCCESS,pnew->path,strlen(pnew->path))，将当前目录路径发送给客户端。
客户端通过 recv_protocal(client_fd, &control_code, path) 接收服务器返回的路径信息，显示给用户。
mkdir创建新的目录：
客户端调用send_protocal(client_fd, MKDIR, cmd + 6, strlen(cmd) - 6)将 MKDIR 控制码以及目录名发送给服务器。
子线程调用recv_protocal(new_fd, &control_code, client_data)接收客户端请求，switch根据不同的控制码（枚举变量）执行不同的命令处理函数
此时执行MKDIR处理函数,检查当前用户在当前目录下文件系统表是否已存在同名文件或目录。如果已存在，则使用 send_protocal 发送失败给客户端；如果不存在，则在文件系统表中插入记录，并向客户端发送成功。
客户端调用 recv_protocal 接收服务器返回的创建文件夹成功还是失败。
cd返回上级目录或切换到指定的目录：
客户端通过send_protocal(client_fd, CD, cmd + 3, strlen(cmd) - 3)将 CD 控制码和目标目录名发送给服务器。
子线程调用recv_protocal(new_fd, &control_code, client_data)接收客户端请求，switch根据不同的控制码（枚举变量）执行不同的命令处理函数
此时执行CD处理函数
如果目标目录为 ".."： 
检查当前是否处于根目录，若不在根目录即path_id != 0，获取上级目录 ID，并更新路径字符串。
子线程 send_protocal 发送成功和新路径给客户端。
如果不是返回上级目录，即进入指定目录： 
检查文件系统表是否存在目标目录。如果存在，当前路径拼接上目标目录名生成新路径，更新 pnew->path和 pnew->path_id，通过 send_protocal 给客户端发送成功和新路径；
客户端通过 recv_protocal 接收服务器返回的控制码和目录路径。如果成功，打印新路径；否则打印“切换失败”。
ls列出当前目录下的所有文件和目录：
客户端通过send_protocal(client_fd,LS,NULL,0)将LS控制码发送给服务器
子线程调用recv_protocal(new_fd, &control_code, client_data)接收客户端请求，switch根据不同的控制码（枚举变量）执行不同的命令处理函数
此时执行LS处理函数，从文件系统表中查询当前目录下所有文件的文件类型、文件名和文件大小，一行一行的发给客户端
客户端接收一行打印一行
rm删除文件或空目录：
客户端调用 send_protocal(client_fd, RM, cmd + 3, strlen(cmd) - 3) 将RM控制码及文件名或目录名发送给服务器。
子线程调用recv_protocal(new_fd, &control_code, client_data)接收客户端请求，switch根据不同的控制码（枚举变量）执行不同的命令处理函数
此时执行RM处理函数，检查目标是否为普通文件或空目录（非空目录会删除失败）。
服务器端使用 send_protocal 将操作结果发送给客户端。
客户端recv_protocal接收服务器返回的结果
puts上传文件，支持秒传机制(根据md5值实现)：
客户端调用 send_protocal(client_fd, PUTS, cmd + 5, strlen(cmd) - 5)
将 puts 控制码和文件名发送给服务器，把计算出的文件 MD5 值发送给服务器。
子线程调用recv_protocal(new_fd, &control_code, client_data)接收客户端请求，switch根据不同的控制码（枚举变量）执行不同的命令处理函数
此时执行PUTS处理函数，接收文件的 MD5 值。更新该md5值文件的引用计数，如果更新成功，在文件系统表中插入一条记录，发送秒传成功给客户端，如果更新失败，说明md5表中没有该文件，发送秒传失败给客户端。
客户端如果收到秒传失败，传文件给服务器端。
服务器接收文件并在文件md5表和文件系统表中各插入一条记录
gets下载文件，支持断点续传功能(根据续传位置实现)：
客户端尝试打开文件，如果打开失败，则将续传位置设为0；
如果打开成功，调用 fstat 获取文件当前大小，将其设为续传位置，即已下载部分的大小。
调用sprinf拼接命令与续传位置组成一个新的命令字符串，其中包含文件名及已下载的大小。
使用 send_protocal(client_fd, GETS, new_cmd + 5, strlen(new_cmd) - 5)
将 gets 控制码、文件名以及续传位置发送给服务器。
子线程调用recv_protocal(new_fd, &control_code, client_data)接收客户端请求，switch根据不同的控制码（枚举变量）执行不同的命令处理函数
此时执行GETS处理函数，从字符串中提取续传位置，同时将字符串中的空格替换成字符串结束符，从而将原字符串拆分成两部分：一部分是文件名（空格之前的部分），另一部分是偏移量字符串（空格之后的部分）
根据当前路径id和文件名查询文件系统表，获取该文件的MD5 值和大小。
transFile从文件的续传位置开始传输剩余数据给客户端。

客户端recvFile接收文件数据并保存

如果续传位置不为0则实现了断点续传下载。

子线程：
子线程进入循环，加锁，如果队列为空，则阻塞等待，如果队列不为空，从任务队列出队一个任务，解锁并处理客户端请求。
子线程进入循环，调用recv_protocal(new_fd, &control_code, client_data)接收客户端请求，switch根据不同的控制码例如 PWD、MKDIR、CD、LS、RM、PUTS、GETS 等（枚举变量）执行不同的命令处理函数。
处理过程中，子线程负责与客户端进行双向通信，直到客户端断开连接或出现错误。
处理完任务后，子线程关闭对应的数据库连接和客户端 socket
