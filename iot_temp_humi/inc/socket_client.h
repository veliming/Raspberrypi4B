/*** 
 * @Author: RoxyKko
 * @Date: 2023-04-04 17:37:02
 * @LastEditors: RoxyKko
 * @LastEditTime: 2023-04-04 17:39:44
 * @Description: socket client 端代码
 */

#ifndef __SOCKET_CLIENT_H__
#define __SOCKET_CLIENT_H__

#include <stdio.h>
#include <sys/types.h>	// for socket
#include <sys/socket.h> // for socket
#include <string.h>		// fpr strerror and memset
#include <errno.h>		// for errno
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>	// for inet_aton
#include <fcntl.h>		// for open
#include <unistd.h>		// for read/write/close
#include <getopt.h>		// for getopt_long
#include <stdlib.h>		// for atoi
#include "iot_main.h"


int socket_client_init(char *serv_ip, int port);


#endif
