/*** 
 * @Author: RoxyKko
 * @Date: 2023-04-04 17:06:27
 * @LastEditors: RoxyKko
 * @LastEditTime: 2023-04-04 17:07:02
 * @Description: 
 */

#ifndef __IOT_MAIN_H__
#define __IOT_MAIN_H__


#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <gpiod.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <syslog.h>

#include "i2c_sht20.h"

# endif