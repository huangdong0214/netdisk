#include "func.h"
//请输入用户名和密码
int input_username_passwd(username_passwd_t *user)
{
    printf("username\n");
    scanf("%s",user->user_name);
    printf("passwd\n");
    scanf("%s",user->passwd);
    return 0;
}