#include "factory.h"


//随机盐值
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LENGTH 19

void generate_random_string(char *random_string) //定义函数generate_random_string，用于生成随机字符串
{
    char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";//可用字符集合  
    int i;

    srand(time(NULL));//设置随机数种子，使用当前时间作为种子  
    
    for (i = 3; i < LENGTH; ++i) {//从下标3开始赋值，预留前面字符或用于其他目的 
        random_string[i] = charset[rand() % (strlen(charset) - 1)];//随机选择字符并赋值  
    }

    random_string[LENGTH] = '\0'; //在字符串末尾添加结束符  
}


off_t get_file_name_pos(char *file_name_and_len)//定义函数get_file_name_pos，用于解析文件名与偏移量
{
    int i;
    for(i=0;i<strlen(file_name_and_len);i++)//遍历字符串，查找空格位置  
    {
        if(file_name_and_len[i]==' ')//如果找到空格  
        {
            break;//跳出循环  
        }
    }
    // 把空格换为\0
    file_name_and_len[i]='\0';//将空格替换为结束符，使前半部分成为独立字符串  
    char seek_pos[100];//定义临时数组用于存储偏移量字符串  
    strcpy(seek_pos,file_name_and_len+i+1);//复制空格后面的内容到seek_pos  
    return atol(seek_pos);//将字符串转换为长整数并返回  
}
