/*
 * @Author: RoxyKko
 * @Date: 2023-03-24 18:50:49
 * @LastEditors: RoxyKko
 * @LastEditTime: 2023-03-24 22:36:10
 * @Description: sht20驱动
 */
#include <i2c_sht20.h>


struct gpiod_chip *chip = NULL;
struct gpiod_line *sda_line = NULL;
struct gpiod_line *scl_line = NULL;

int main(int argc, char **argv)
{
    int i2c_fd;
    float temperature = 0;
    float humidity = 0;
    int u_temp_data = 0;
    int u_humi_data = 0;

    i2c_fd = sht20_init();
    if(i2c_fd <= 0)
    {
        printf("sht20_init() failure\n");
        return -1;
    }
    printf("i2c_fd[%d] open\n",i2c_fd);

    while(1)
    {
        if( ( u_temp_data = get_temp_hold(i2c_fd) ) < 0 )
        {
            break;
        }
        if( ( u_humi_data = get_humi_hold(i2c_fd) ) < 0 )
        {
            break;
        }

        temperature = -46.85 + 175.72 * u_temp_data / 65536.0;
        humidity = -6.0 + 125.0 * u_humi_data / 65536.0;
        printf("Temperature: %.2fC\n", temperature);
        printf("Humidity: %.2f%%\n", humidity);
        sleep(5);
    }
    close(i2c_fd);
    gpiod_chip_close(chip);
    gpiod_line_release(sda_line);
    gpiod_line_release(scl_line);
    gpiod_chip_close(chip);
    return 0;
}

/**
 * @name: int sht20_init()
 * @description: 初始化sht20
 * @return {*}
 */
int sht20_init()
{
    int _i2c_fd;

    /***********************  引脚初始化  ***********************/

    chip = gpiod_chip_open(CHIP_NAME);
    if (chip == NULL)
    {
        printf("Error opening GPIO chip\n");
        gpiod_chip_close(chip);
        return -1;
    }

    sda_line = gpiod_chip_get_line(chip, 2); // SDA = GPIO2
    if (sda_line == NULL)
    {
        printf("Error getting SDA GPIO line\n");
        gpiod_chip_close(chip);
        return -2;
    }

    scl_line = gpiod_chip_get_line(chip, 3); // SCL = GPIO3
    if (scl_line == NULL)
    {
        printf("Error getting SCL GPIO line\n");
        gpiod_chip_close(chip);
        return -2;
    }

    gpiod_line_request_output(sda_line, "i2c", 0);
    gpiod_line_request_output(scl_line, "i2c", 0);

    gpiod_line_set_value(sda_line, 1);
    gpiod_line_set_value(scl_line, 1);

    /***********************  引脚初始化  ***********************/

    /***********************  I2C初始化  ***********************/
    // 打开_i2c_fd,可读可写
    if ((_i2c_fd = open(I2C_BUS, O_RDWR)) < 0)
    {
        printf("Error opening I2C device\n");
        gpiod_line_release(sda_line);
        gpiod_line_release(scl_line);
        gpiod_chip_close(chip);
        return -3;
    }

    // 设置i2c设备为从机
    if (ioctl(_i2c_fd, I2C_SLAVE, SHT20_ADDR) < 0)
    {
        printf("Error setting I2C slave address\n");
        close(_i2c_fd);
        gpiod_line_release(sda_line);
        gpiod_line_release(scl_line);
        gpiod_chip_close(chip);
        return -3;
    }

    /***********************  I2C初始化  ***********************/

    return _i2c_fd;
}

/**
 * @name: sht20_delay()
 * @description: sht20延时
 * @return {*}
 */
void sht20_delay()
{
    usleep(SHT20_MEASURING_DELAY * 1000); // 15ms
}

/**
 * @name: static void i2c_start()
 * @description: 启动i2c,开始传输
 * @return {*}
 */
static void i2c_start()
{
    gpiod_line_set_value(sda_line, 0);
    gpiod_line_set_value(scl_line, 1);
    gpiod_line_set_value(sda_line, 1);
    gpiod_line_set_value(scl_line, 0);
}

/**
 * @name: int get_temp_hold(int _i2c_fd)
 * @description: 使传感器设为温度触发保持模式，并获得温度数据
 * @param {int} _i2c_fd ：i2c文件描述符
 * @return {int} temp_data ：温度数据
 */
int get_temp_hold(int _i2c_fd)
{
    // 温度触发保持模式
    if (i2c_smbus_write_byte(_i2c_fd, CMD_TRIGGER_TEMP_HOLD) < 0)
    {
        printf("Error sending I2C command\n");
        return -1;
    }

    sht20_delay();

    // 读温度数据
    int temp_data = i2c_smbus_read_word_data(_i2c_fd, 0);
    if (temp_data < 0)
    {
        printf("Error reading temperature data\n");
        return -1;
    }

    return temp_data;
}

/**
 * @name: int get_humi_hold(int _i2c_fd)
 * @description: 使传感器设为湿度触发保持模式，并获得温度数据
 * @param {int} _i2c_fd ：i2c文件描述符
 * @return {*}humi_data ：湿度数据
 */
int get_humi_hold(int _i2c_fd)
{
    // 湿度触发保持模式
    if (i2c_smbus_write_byte(_i2c_fd, CMD_TRIGGER_HUMI_HOLD) < 0)
    {
        printf("Error sending I2C command\n");
        return -1;
    }

    sht20_delay();

    // 读湿度数据
    int humi_data = i2c_smbus_read_word_data(_i2c_fd, 0);
    if (humi_data < 0)
    {
        printf("Error reading temperature data\n");
        return -1;
    }

    return humi_data;
}