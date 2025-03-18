#include "factory.h"//引入头文件 "factory.h"，包含相关数据结构和函数声明。

//为每个子线程初始化一个连接//说明该函数用于初始化MySQL连接
int mysql_connect(MYSQL **conn, int port)//定义mysql_connect函数，参数conn为MYSQL连接指针地址，port为端口号
{
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    const char* server = "localhost";//定义服务器地址为"localhost"
    const char* user = "root";//定义数据库用户名为"root"
    const char* password = "123";//定义数据库密码为"123"
    const char* database = "newdisk";//定义要访问的数据库名称为"newdisk"
    *conn = mysql_init(NULL);//初始化MYSQL连接
    if (!mysql_real_connect(*conn, server, user, password, database, port, NULL, 0))//尝试连接数据库
    {
        printf("Error connecting to database:%s\n", mysql_error(*conn));//打印数据库连接错误信息
        return -1;//返回-1表示连接失败
    } else {
#ifdef DEBUG
        printf("Connected...\n");//在DEBUG模式下打印连接成功信息
#endif
        return 0;//返回0表示连接成功
    }
}

//检查用户名在数据库中是否存在
int check_username_exist(MYSQL *conn, char *username)//定义check_username_exist函数，参数conn为MYSQL连接，username为待检测的用户名
{
    unsigned int t;
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    int return_flag;
    char *p = "select user_name from users where user_name='";//定义SQL查询语句的一部分
    char query[300] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%s'", p, username);//将用户名拼接进sql查询语句
    puts(query);//调试
    t = mysql_query(conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印查询错误信息
    } else {
        printf("Query made...\n");//打印查询成功信息
        res = mysql_use_result(conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            if (NULL == row)//如果没有查询到数据
            {
                return_flag = 0;//设置返回标志为0，表示用户名不存在
            }
            else {
                return_flag = 1;//设置返回标志为1，表示用户名存在
            }
        }
        mysql_free_result(res);//释放查询结果资源
    }
    return return_flag;//返回查询结果标志
}

//注册时用户名不在数据库，将用户名及加密密码存入数据库
uint64_t insert_new_user(MYSQL *conn, username_passwd_t *p)//定义insert_new_user函数，参数conn为MYSQL连接，p为用户名和密码结构体指针
{
    unsigned int t;//定义变量t用于存储mysql_query的返回值
    char salt[20] = "$6$";//定义并初始化salt字符串，初始部分为"$6$"
    generate_random_string(salt);//生成随机salt字符串，往$6$后面拼接16个随机字符
    struct crypt_data cr_data;//定义crypt_data结构体变量，用于存储加密密码
    crypt_r(p->passwd, salt, &cr_data);//对用户密码进行加密，得到密文
    char *psql = "insert into users VALUES(0,'";//定义SQL插入语句前缀
    char query[1000] = {0};//定义SQL查询语句缓冲区并初始化为0
    sprintf(query, "%s%s','%s','%s')", psql, p->user_name, salt, cr_data.output);//格式化SQL插入语句，将用户名、salt和密文填入
    puts(query);//打印生成的SQL语句
    t = mysql_query(conn, query);//执行SQL插入语句
    if (t)//如果执行失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印错误信息
        return 0;//返回0表示插入失败
    } else {
        return mysql_affected_rows(conn);//返回受影响的行数，表示插入成功
    }
}

//检查用户登录时输入的用户名和盐值密文是否已在数据库
int check_login(MYSQL *conn, username_passwd_t *p)//定义check_login函数，参数conn为MYSQL连接，p为用户名和密码结构体指针
{
    unsigned int t;
    int login_flag;
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    struct crypt_data cr_data;//定义crypt_data结构体变量，用于存储密码加密结果
    char *psql = "SELECT salt, ciphertext FROM users WHERE user_name = '";//定义SQL查询语句前缀
    char query[1000] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%s'", psql, p->user_name);//格式化SQL查询语句，将用户名填入
    puts(query);//打印生成的SQL查询语句
    t = mysql_query(conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印查询错误信息
    } else {
        printf("Query made...\n");//打印查询成功信息
        res = mysql_use_result(conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            if (row)//如果查询到数据
            {
                crypt_r(p->passwd, row[0], &cr_data);//用用户输入的密码与 row[0]（盐值）通过 crypt_r 重新生成散列，然后将生成的散列与 row[1] 进行比较，确认密码是否匹配。
                if (!strcmp(cr_data.output, row[1]))//比较生成的密文与数据库中的密文是否一致
                {
                    login_flag = 1;//设置login_flag为1，表示登录成功
                } else {
                    login_flag = 0;//有用户名，密码不对
                }
            }
            else {
                login_flag = 0;//没有用户名
            }
        }
        mysql_free_result(res);//释放查询结果资源
    }
    return login_flag;//返回登录验证结果
}

//根据用户名查询对应的用户ID
int get_user_id(MYSQL *conn, char *username)//定义get_user_id函数，参数conn为MYSQL连接，username为待查询的用户名
{
    unsigned int t;
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    int return_id;
    char *p = "select user_id from users where user_name='";//定义SQL查询语句前缀
    char query[300] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%s'", p, username);//格式化SQL查询语句，将用户名填入
    puts(query);//打印生成的SQL查询语句
    t = mysql_query(conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印查询错误信息
    } else {
        printf("Query made...\n");//打印查询成功信息
        res = mysql_use_result(conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            if (NULL == row)//如果没有查询到数据
            {
                return_id = 0;//返回0表示未查到用户ID
            }
            else {
                return_id = atoi(row[0]);//将查询到的用户ID字符串转换为整数
            }
        }
        mysql_free_result(res);//释放查询结果资源
    }
    return return_id;//返回用户ID
}

//检测指定目录是否存在于文件系统中
int check_dir_isexist(MYSQL* conn, int user_id, int path_id, char* dir_name)//定义check_dir_isexist函数，参数conn为MYSQL连接，user_id为用户ID，path_id为父目录ID，dir_name为目录名
{
    unsigned int t;
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    int return_id;//定义返回变量，用于存储查询结果
    char *p = "select count(*) from file_system where user_id=";//定义SQL查询语句前缀
    char query[300] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%d and pre_id=%d and file_name='%s'", p, user_id, path_id, dir_name);//格式化SQL查询语句，将参数填入
    puts(query);//打印生成的SQL查询语句
    t = mysql_query(conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印查询错误信息
    } else {
        printf("Query made...\n");//打印查询成功信息
        res = mysql_use_result(conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            if (NULL == row)//如果未查询到数据
            {
                return_id = 0;//设置返回值为0
            }
            else {
                return_id = atoi(row[0]);//将查询到的结果转换为整数
            }
        }
        mysql_free_result(res);//释放查询结果资源
    }
    return return_id;//返回查询结果
}

//在文件系统中创建一个文件夹
uint64_t creat_dir(MYSQL* conn, int user_id, int path_id, char* dir_name)//定义creat_dir函数，参数conn为MYSQL连接，user_id为用户ID，path_id为父目录ID，dir_name为新目录名称
{
    unsigned int t;
    char *psql = "INSERT INTO file_system (pre_id, file_name, user_id, file_type, file_size) VALUES (";//定义SQL插入语句前缀
    char query[1000] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%d,'%s',%d, 'd', 4096)", psql, path_id, dir_name, user_id);//格式化SQL语句，将参数填入，'d'表示目录类型
    puts(query);//打印生成的SQL语句
    t = mysql_query(conn, query);//执行SQL插入语句
    if (t)//如果执行失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印错误信息
        return 0;//返回0表示创建失败
    } else {
        return mysql_affected_rows(conn);//返回受影响的行数，表示创建成功
    }
}

//判断cd的文件夹是否存在
int check_cd_dir_isexist(MYSQL* conn, int user_id, int path_id, char* dir_name)//定义check_cd_dir_isexist函数，参数conn为MYSQL连接，user_id为用户ID，path_id为当前目录ID，dir_name为目标目录名
{
    unsigned int t;
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    int ret_path_id;//定义返回的目录ID变量
    char *p = "select path_id from file_system where user_id=";//定义SQL查询语句前缀
    char query[300] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%d and pre_id=%d and file_name='%s' and file_type='d'", p, user_id, path_id, dir_name);//格式化SQL查询语句，将参数填入，并限定文件类型为目录'd'
    puts(query);//打印生成的SQL查询语句
    t = mysql_query(conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印查询错误信息
    } else {
        printf("Query made...\n");//打印查询成功信息
        res = mysql_use_result(conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            if (NULL == row)//如果未查询到数据
            {
                ret_path_id = 0;//设置返回目录ID为0
            }
            else {
                ret_path_id = atoi(row[0]);//将查询到的目录ID字符串转换为整数
            }
        }
        mysql_free_result(res);//释放查询结果资源
    }
    return ret_path_id;//返回目录ID
}

//获取当前目录的上一级目录ID
int get_pre_id(MYSQL* conn, int path_id)//定义get_pre_id函数，参数conn为MYSQL连接，path_id为当前目录ID
{
    unsigned int t;//定义变量t用于存储mysql_query的返回值
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    int ret_pre_id;//定义返回的前置目录ID变量
    char *p = "select pre_id from file_system where path_id=";//定义SQL查询语句前缀
    char query[300] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%d", p, path_id);//格式化SQL查询语句，将path_id填入
    puts(query);//打印生成的SQL查询语句
    t = mysql_query(conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印查询错误信息
    } else {
        printf("Query made...\n");//打印查询成功信息
        res = mysql_use_result(conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            if (NULL == row)//如果未查询到数据
            {
                ret_pre_id = 0;//返回pre_id为0
            }
            else {
                ret_pre_id = atoi(row[0]);//将查询到的前置目录ID字符串转换为整数
            }
        }
        mysql_free_result(res);//释放查询结果资源
    }
    return ret_pre_id;//返回前置目录ID
}

//用于ls查一行发一行给客户端
void send_file_info(task_t* pnew)//定义send_file_info函数，参数pnew为任务结构体指针
{
    train_t train;
    unsigned int t;//定义变量t用于存储mysql_query的返回值
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    char *p = "select file_type,file_name,file_size from file_system where pre_id=";//定义SQL查询语句前缀
    char query[300] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%d", p, pnew->path_id);//格式化SQL查询语句，将当前目录ID填入
    t = mysql_query(pnew->conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(pnew->conn));//打印查询错误信息
    } 
    else {
        printf("Query made...\n");//打印查询成功信息
        res = mysql_use_result(pnew->conn);//获取查询结果
        if (res)
        {
            while ((row = mysql_fetch_row(res)) != NULL)//遍历查询结果的每一行
            {    
                bzero(&train, sizeof(train_t));//清空train结构体内存
                sprintf(train.buf, "%5s%15s%10s", row[0], row[1], row[2]);//格式化文件信息到train.buf中
                train.size = strlen(train.buf);//设置发送数据的大小
                send(pnew->new_fd, &train, 4 + train.size, 0);//发送数据到客户端
            }
            int end_flag = 0;//定义结束标志变量，表示发送结束
            send(pnew->new_fd, &end_flag, 4, 0);//发送结束标志给客户端
        } 
        else {
            printf("Don't find data\n");//打印未查询到数据的提示
        }
        mysql_free_result(res);//释放查询结果资源
    }
}

//删除指定的文件或空目录，并在删除普通文件时更新文件 MD5 值对应的引用计数
int rm_file_emptydir(task_t *pnew, char* rm_file_name)//定义rm_file_emptydir函数，参数pnew为任务结构体指针，rm_file_name为要删除的文件或目录名称
{
    unsigned int t;
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    int rm_flag;//定义rm_flag变量，用于判断删除条件
    char *p = "select count(*) from file_system where pre_id=(select path_id from file_system where pre_id=";//定义SQL查询语句前缀，用于检查是否有子项
    char query[300] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%d and file_name='%s')", p, pnew->path_id, rm_file_name);//格式化SQL查询语句，将当前路径ID和文件名填入
    puts(query);//打印生成的SQL查询语句
    t = mysql_query(pnew->conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(pnew->conn));//打印查询错误信息
    } else {
        printf("Query made...\n");//打印查询成功信息
        res = mysql_use_result(pnew->conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            if (NULL == row)//如果未查询到数据
            {
                rm_flag = 0;//设置rm_flag为0
            }
            else {
                rm_flag = atoi(row[0]);//将查询到的计数转换为整数赋值给rm_flag
            }
        }
        mysql_free_result(res);//释放查询结果资源
    }
    //判断删除的是否是普通文件，以便更新引用计数
    char md5_str[50] = {0};//定义md5_str数组并初始化为0，用于存储文件的md5值
    p = "select file_type,md5 from file_system where pre_id=";//查询文件类型和md5值
    bzero(query, sizeof(query));//清空查询语句缓冲区
    sprintf(query, "%s%d and file_name='%s'", p, pnew->path_id, rm_file_name);//格式化SQL查询语句，将当前路径ID和文件名填入
    puts(query);//打印生成的SQL查询语句
    t = mysql_query(pnew->conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(pnew->conn));//打印查询错误信息
    } else {
        res = mysql_use_result(pnew->conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            if (NULL == row)
            {
                //如果用户输入的名字不对，则不做任何操作
            }
            else {
                if (row[0][0] == 'f')//row[0]是字符串，row[0][0]是字符串的第一个字符，如果文件类型为普通文件（'f'）
                {
                    strcpy(md5_str, row[1]);//将文件的md5值复制到md5_str中
                }
            }
        }
        mysql_free_result(res);//释放查询结果资源
    }
    //删除普通文件或空目录
    if (!rm_flag)//如果rm_flag为0，即没有子项存在
    {
        p = "delete from file_system where pre_id=";//定义SQL删除语句前缀
        bzero(query, sizeof(query));//清空查询语句缓冲区
        sprintf(query, "%s%d and file_name='%s'", p, pnew->path_id, rm_file_name);//格式化SQL删除语句，将当前路径ID和文件名填入
        t = mysql_query(pnew->conn, query);//执行SQL删除语句
        if (t)//如果删除失败
        {
            printf("Error making query:%s\n", mysql_error(pnew->conn));//打印删除错误信息
        } else {
            printf("delete success,delete row=%ld\n", (long)mysql_affected_rows(pnew->conn));//打印删除成功信息及受影响行数
        }
    }
    //如果是普通文件，就需要file_md5sum的引用计数减1
    if (md5_str[0])//如果md5_str不等于0
    {
        p = "update file_md5sum set reference_count=reference_count-1 where md5='";//定义SQL更新语句前缀
        bzero(query, sizeof(query));//清空查询语句缓冲区
        sprintf(query, "%s%s'", p, md5_str);//格式化SQL更新语句，将md5值填入
        puts(query);//打印生成的SQL语句
        t = mysql_query(pnew->conn, query);//执行SQL更新语句
        if (t)//如果更新失败
        {
            printf("Error making query:%s\n", mysql_error(pnew->conn));//打印更新错误信息
        }
    }
    return !rm_flag;//返回!rm_flag，表示删除操作是否成功
}

//检查文件md5是否已存在，并更新引用计数
uint64_t check_file_md5_exist(task_t* pnew, char* md5_str)//定义check_file_md5_exist函数，参数pnew为任务结构体指针，md5_str为文件md5值
{
    unsigned int t;//定义变量t用于存储mysql_query的返回值
    char *psql = "update file_md5sum set reference_count=reference_count+1 where md5='";//定义SQL更新语句前缀
    char query[1000] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%s'", psql, md5_str);//格式化SQL语句，将md5值填入
    puts(query);//打印生成的SQL语句
    t = mysql_query(pnew->conn, query);//执行SQL更新语句
    if (t)//如果更新失败
    {
        printf("Error making query:%s\n", mysql_error(pnew->conn));//打印更新错误信息
        return 0;//返回0表示md5不存在或更新失败
    } else {
        return mysql_affected_rows(pnew->conn);//返回受影响的行数，1表示md5存在且更新成功，0表示不存在
    }
}

//在file_system中插入文件记录
uint64_t creat_file(MYSQL* conn, int user_id, int path_id, char* file_name, char* md5_str)//定义creat_file函数，参数conn为MYSQL连接，user_id为用户ID，path_id为父目录ID，file_name为文件名，md5_str为文件md5值
{
    unsigned int t;//定义变量t用于存储mysql_query的返回值
    char *psql = "INSERT INTO file_system (pre_id, file_name, user_id, file_type,md5, file_size) VALUES (";//定义SQL插入语句前缀
    char query[1000] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%d,'%s',%d, 'f','%s', (select file_size from file_md5sum where md5='%s'))", psql, path_id, file_name, user_id, md5_str, md5_str);//格式化SQL语句，将各参数填入，'f'表示普通文件
    puts(query);//打印生成的SQL语句
    t = mysql_query(conn, query);//执行SQL插入语句
    if (t)//如果执行失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印错误信息
        return 0;//返回0表示创建失败
    } else {
        return mysql_affected_rows(conn);//返回受影响的行数，表示文件记录创建成功
    }
}

//在文件md5sum表中创建新记录
uint64_t creat_file_md5(MYSQL* conn, char* md5_str, off_t file_size)//定义creat_file_md5函数，参数conn为MYSQL连接，md5_str为文件md5值，file_size为文件大小
{
    unsigned int t;//定义变量t用于存储mysql_query的返回值
    char *psql = "insert into file_md5sum VALUES(";//定义SQL插入语句前缀
    char query[1000] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s'%s',1,%ld)", psql, md5_str, file_size);//格式化SQL语句，将md5值和文件大小填入，初始引用计数为1
    puts(query);//打印生成的SQL语句
    t = mysql_query(conn, query);//执行SQL插入语句
    if (t)//如果执行失败
    {
        printf("Error making query:%s\n", mysql_error(conn));//打印错误信息
        return 0;//返回0表示创建失败
    } else {
        return mysql_affected_rows(conn);//返回受影响的行数，表示记录创建成功
    }
}

//获取文件的md5和文件大小
void get_md5_and_file_size(task_t* pnew, char* md5_str, off_t* file_size, char* file_name)//定义get_md5_and_file_size函数，参数pnew为任务结构体指针，md5_str用于存储md5值，file_size用于存储文件大小，file_name为待查询文件名
{
    unsigned int t;//定义变量t用于存储mysql_query的返回值
    MYSQL_RES *res;//定义MYSQL_RES指针，用于存储查询结果
    MYSQL_ROW row;//定义MYSQL_ROW变量，用于存储查询结果中的一行数据
    char *p = "select md5,file_size from file_system where pre_id=";//定义SQL查询语句前缀
    char query[300] = {0};//定义查询语句缓冲区并初始化为0
    sprintf(query, "%s%d and file_name='%s'", p, pnew->path_id, file_name);//格式化SQL查询语句，将当前路径ID和文件名填入
    t = mysql_query(pnew->conn, query);//执行SQL查询语句
    if (t)//如果查询失败
    {
        printf("Error making query:%s\n", mysql_error(pnew->conn));//打印查询错误信息
    } else {
        res = mysql_use_result(pnew->conn);//获取查询结果
        if (res)
        {
            row = mysql_fetch_row(res);//获取查询结果中的一行数据
            strcpy(md5_str, row[0]);//将查询到的md5值复制到md5_str中
            *file_size = atol(row[1]);//将查询到的文件大小字符串转换为长整型，并存入file_size
        } else {
            printf("Don't find data\n");//打印未查询到数据的提示
        }
        mysql_free_result(res);//释放查询结果资源
    }
}