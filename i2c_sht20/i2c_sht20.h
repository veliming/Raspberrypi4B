/*** 
 * @Author: RoxyKko
 * @Date: 2023-03-24 19:10:08
 * @LastEditors: RoxyKko
 * @LastEditTime: 2023-03-24 20:56:17
 * @Description: sht20驱动
 */
#ifndef _I2C_SHT20_H_
#define _I2C_SHT20_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <gpiod.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <errno.h>

#define I2C_BUS                     "/dev/i2c-1"    // 设备名
#define SHT20_ADDR                  0x40            // i2c设备物理地址
#define CHIP_NAME                   "/dev/gpiochip0"     // GPIO NAME

#define CMD_TRIGGER_TEMP_HOLD       0xE3            // 温度触发保持         8'b1110’0011 = 0xE3
#define CMD_TRIGGER_HUMI_HOLD       0xE4            // 湿度触发保持         8'b1110’0101 = 0xE4
#define CMD_TRIGGER_TEMP_NOHOLD     0xF3            // 温度触发不保持       8'b1111’0011 = 0xF3
#define CMD_TRIGGER_HUMI_NOHOLD     0xF4            // 湿度触发不保持       8'b1111’0101 = 0xF4
#define CMD_WRITE_USER_REG          0xE6            // 写用户寄存器         8'b1110’0110 = 0xE6
#define CMD_READ_USER_REG           0xE7            // 读用户寄存器         8'b1110’0111 = 0xE7
#define CMD_SOFT_RESET              0xFE            // 软复位               8'b1111’1110 = 0xFE

#define SHT20_MEASURING_DELAY 15                    // 上升沿延迟 15ms

int sht20_init();
void sht20_delay();
int get_temp_hold(int _i2c_fd);                     // 使传感器设为温度触发保持模式，并获得温度数据
int get_humi_hold(int _i2c_fd);                     // 使传感器设为湿度触发保持模式，并获得温度数据


#endif