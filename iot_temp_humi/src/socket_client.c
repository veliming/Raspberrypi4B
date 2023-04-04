/*
 * @Author: RoxyKko
 * @Date: 2023-04-04 18:38:48
 * @LastEditors: RoxyKko
 * @LastEditTime: 2023-04-04 18:38:50
 * @Description: 
 */

#include "socket_client.h"

int socket_client_init(char *serv_ip, int port)
{
    int socket_fd;
    struct sockaddr_in servaddr;
    int rv = -1;

    // 创建socket并获得socket描述符
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    // 创建socket失败
    if (socket_fd < 0)
    {
        printf("Create socket failure: %s\n", strerror(errno));
        return -1;
    }

    // 创建socket成功
    printf("Create socket[%d] successfully\n", socket_fd);

    // 初始化结构体，将空余的8位字节填充为0
    // 设置参数，connect连接服务器
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;                 // IPV4
    servaddr.sin_port        = htons(port);             // port
    inet_aton(serv_ip, &servaddr.sin_addr);             // 将点分十进制IP转换为网络字节序
    rv = connect(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // 连接服务器失败
    if (rv < 0)
    {
        printf("Connect to server[%s:%d] failure : %s\n",
               serv_ip, port, strerror(errno));
        return -2;
    }
    // 连接服务器成功,打印服务器IP地址和端口号
    printf("Connect to server[%s:%d] successfully!\n", serv_ip, port);

    // 若链接成功则返回socket描述符
    return socket_fd;
}