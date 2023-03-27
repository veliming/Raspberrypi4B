/*
 * @Author: RoxyKko
 * @Date: 2023-03-26 11:22:00
 * @LastEditors: RoxyKko
 * @LastEditTime: 2023-03-27 23:20:12
 * @Description: iot项目-温湿度检测
 */
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

#define I2C_BUS "/dev/i2c-1"           // 设备名
#define SHT20_ADDR 0x40                // i2c设备物理地址
#define Vision 1.0                     // 版本号
#define lastEdit "2023-03-27 22:33:51" // 最后编辑时间

#define CMD_TRIGGER_TEMP_HOLD 0xE3   // 温度触发保持         8'b1110’0011 = 0xE3
#define CMD_TRIGGER_HUMI_HOLD 0xE5   // 湿度触发保持         8'b1110’0101 = 0xE5
#define CMD_TRIGGER_TEMP_NOHOLD 0xF3 // 温度触发不保持       8'b1111’0011 = 0xF3
#define CMD_TRIGGER_HUMI_NOHOLD 0xF5 // 湿度触发不保持       8'b1111’0101 = 0xF5
#define CMD_WRITE_USER_REG 0xE6      // 写用户寄存器         8'b1110’0110 = 0xE6
#define CMD_READ_USER_REG 0xE7       // 读用户寄存器         8'b1110’0111 = 0xE7
#define CMD_SOFT_RESET 0xFE          // 软复位               8'b1111’1110 = 0xFE

int sht2x_init(void);
int sht2x_softReset(int fd);
int sht2x_get_temp_humidity(int fd, float *temp, float *rh);
int sht2x_get_serialNumber(int fd, uint8_t *serialNumber, int size);
static inline void msleep(unsigned long ms);
static inline void dump_buf(const char *prompt, uint8_t *buf, int size);
static inline void print_usage(char *progname);
static inline void print_vision(char *progname);
int socket_client_init(char *serv_ip, int port);

int main(int argc, char **argv)
{
    int opt;               // 命令行选项
    int i2c_fd;            // i2c设备文件描述符
    int daemon_run = 0;    // 后台运行标志
    char *progname = NULL; // 程序名
    int error = -1;        // 报错提示符
    float temp, rh;        // 温度、湿度
    char *servip = NULL;
    int port = 0;
    char buf[1024];
    int socket_fd;
    bool socket_connected = false;

    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"temp", no_argument, NULL, 't'},
        {"humi", no_argument, NULL, 'H'},
        {"serial", no_argument, NULL, 's'},
        {"deamon", no_argument, NULL, 'b'},
        {"ipaddr", required_argument, NULL, 'i'},
        {"port", required_argument, NULL, 'p'},
        {0, 0, 0, 0}};

    progname = argv[0];

    // 命令行选项解析
    while ((opt = getopt_long(argc, argv, "hvtHsbp:i:", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'h':
            // 显示帮助信息
            print_usage(progname);
            return EXIT_SUCCESS;
        case 'v':
            // 显示版本信息
            print_vision(progname);
            return EXIT_SUCCESS;
        case 't':
            // 显示温度
            break;
        case 'H':
            // 显示湿度
            break;
        case 's':
            // 显示序列号
            break;
        case 'b':
            // 后台运行
            daemon_run = 1;
            break;
        case 'i':
            // 获取IP
            servip = optarg;
            break;
        case 'p':
            // 获取端口号
            port = atoi(optarg);
            break;
        default:
            break;
        }
    }

    if (!servip || !port)
    {
        print_usage(argv[0]);
        return 0;
    }

    if (daemon_run)
    {
        daemon(0, 0);
    }

    // 初始化sht20
    i2c_fd = sht2x_init();
    if (i2c_fd < 0)
    {
        printf("sht2x initialize failed!\n");
        return 1;
    }
    printf("sht2x initialize success!\n");

    // sht20软件复位
    if (sht2x_softReset(i2c_fd) < 0)
    {
        printf("sht2x softReset failed!\n");
        return 2;
    }
    printf("sht2x softReset success!\n");

    while (1)
    {
        // 温湿度采样
        if (sht2x_get_temp_humidity(i2c_fd, &temp, &rh) < 0)
        {
            printf("sht2x get temp and humidity failed!\n");
            return 4;
        }

        // 若socket未连接，则连接socket
        if (!socket_connected)
        {
            socket_fd = socket_client_init(servip, port);
            if (socket_fd < 0)
            {
                // 待做：将温湿度数据存入SQLite数据库
                continue;
            }
            socket_connected = true;
        }

        // 将温湿度数据写入给服务器
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "Temp: %f C, Humi: %f %%\r\n", temp, rh);
        if (write(socket_fd, buf, strlen(buf)) < 0)
        {
            // 若写入失败，则说明socket连接出现错误
            socket_connected = false;
            close(socket_fd);
            // 待做：将温湿度数据存入SQLite数据库
            continue;
        }

        // 检查SQLlite数据库中是否存在数据
        // 若存在，则将数据写入给服务器
        // 若不存在，则不做任何操作
    }
}

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
    servaddr.sin_family = AF_INET;          // IPV4
    servaddr.sin_port = htons(port);        // port
    inet_aton(serv_ip, &servaddr.sin_addr); // 将点分十进制IP转换为网络字节序
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

/**
 * @name: static inline void print_usage(char *progname)
 * @description: 打印帮助信息
 * @param {char} *progname 程序名称
 * @return {*}
 */
static inline void print_usage(char *progname)
{
    printf("Usage: %s [OPTION] ...\n", progname);

    printf(" %s 这是一个温湿度检测传感器iot项目 \n", progname);
    printf("\nMandatory arguments to long options are mandatory for short option too:\n");

    printf(" -i[ipaddr ]: sepcify server IP address\n");
    printf(" -p[port   ]: sepcify server port \n");
    printf(" -b[daemon ] set program running on background\n");
    printf(" -p[port   ] Socket server port address\n");
    printf(" -h[help   ] Display this help information\n");
    printf(" -t[temp   ] Display now temp\n");
    printf(" -H[humi   ] Display now humi\n");
    printf(" -v[vision ] Display prog vision\n");

    printf("\nExample: %s -b -p 8900 -i 127.0.0.1\n", progname);
    return;
}

/**
 * @name: static inline void print_vision(char *progname)
 * @description: 打印程序版本号
 * @param {char} *progname 程序名
 * @return {*}
 */
static inline void print_vision(char *progname)
{
    printf("Usage: %s [VISION] ...\n", progname);
    printf(" %s 该程序的版本为： %f 最后更新日期为： %s\n", progname, Vision, lastEdit);
    return;
}

/**
 * @name: static inline void msleep(unsigned long ms)
 * @description: 休眠函数
 * @param {unsigned long} ms
 * @return {*}
 */
static inline void msleep(unsigned long ms)
{
    struct timespec cSleep;
    unsigned long ulTmp;

    cSleep.tv_sec = ms / 1000;

    if (cSleep.tv_sec == 0)
    {
        ulTmp = ms * 10000;
        cSleep.tv_nsec = ulTmp * 100;
    }
    else
    {
        cSleep.tv_nsec = 0;
    }
    nanosleep(&cSleep, 0);
}

/**
 * @name: static inline void dump_buf(const char *prompt, uint8_t *buf, int len)
 * @description: 打印缓冲区
 * @param {char} *prompt 提示
 * @param {uint8_t} *buf 缓冲区
 * @param {int} len 长度
 * @return {*}
 */
static inline void dump_buf(const char *prompt, uint8_t *buf, int size)
{
    int i;

    if (!buf)
    {
        return;
    }

    if (prompt)
    {
        printf("%s", prompt);
    }

    for (i = 0; i < size; i++)
    {
        printf("%02x ", buf[i]);
    }
    printf("\n");

    return;
}

/**
 * @name: int sht2x_init(void)
 * @description: 初始化sht20
 * @return fd 返回文件描述符
 */
int sht2x_init(void)
{
    int fd;

    if ((fd = open("/dev/i2c-1", O_RDWR)) < 0)
    {
        printf("i2c device open failed: %s\n", strerror(errno));
        return -1;
    }

    if (sht2x_softReset(fd) < 0)
    {
        printf("sht2x softReset failed!\n");
        return -2;
    }

    return fd;
}

/**
 * @name: int sht2x_softReset(int fd)
 * @description: sht20软件复位
 * @param {int} fd
 * @return {*}
 */
int sht2x_softReset(int fd)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data sht2x_data;
    uint8_t buf[2];

    if (fd < 0)
    {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__);
        return -1;
    }

    msg.addr = 0x40;
    msg.flags = 0; // write
    msg.len = 1;
    msg.buf = buf;
    msg.buf[0] = CMD_SOFT_RESET;

    sht2x_data.nmsgs = 1;
    sht2x_data.msgs = &msg;

    if (ioctl(fd, I2C_RDWR, &sht2x_data) < 0)
    {
        printf("%s() ioctl failure: %s\n", __func__, strerror(errno));
        return -2;
    }

    msleep(50);
    return 0;
}

/**
 * @name: int sht2x_get_temp_humidity(int fd, float *temp, float *rh)
 * @description: 从sht20获取温度和湿度并打印
 * @param {int} fd  文件描述符
 * @param {float} *temp 温度参数
 * @param {float} *rh   湿度参数
 * @return {*}
 */
int sht2x_get_temp_humidity(int fd, float *temp, float *rh)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data sht2x_data;
    uint8_t buf[4];

    if (fd < 0)
    {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__);
        return -1;
    }

    /*+------------------------------------------+
     *| measure and get temperature |
     *+------------------------------------------+*/

    msg.addr = 0x40;
    msg.flags = 0; // write
    msg.len = 1;
    msg.buf = buf;
    msg.buf[0] = CMD_TRIGGER_TEMP_NOHOLD; /* trigger temperature without hold I2C bus */

    sht2x_data.nmsgs = 1;
    sht2x_data.msgs = &msg;

    if (ioctl(fd, I2C_RDWR, &sht2x_data) < 0)
    {
        printf("%s() ioctl failure: %s\n", __func__, strerror(errno));
        return -2;
    }

    msleep(85);

    memset(buf, 0, sizeof(buf));
    msg.addr = 0x40;
    msg.flags = I2C_M_RD; // write
    msg.len = 3;
    msg.buf = buf;

    sht2x_data.nmsgs = 1;
    sht2x_data.msgs = &msg;

    if (ioctl(fd, I2C_RDWR, &sht2x_data) < 0)
    {
        printf("%s() ioctl failure: %s\n", __func__, strerror(errno));
        return -2;
    }

    // dump_buf("Temperature sample data: ", buf, 3);
    *temp = 175.72 * (((((int)buf[0]) << 8) + buf[1]) / 65536.0) - 46.85;

    /*+------------------------------------------+
     *| measure and get relative humidity |
     *+------------------------------------------+*/

    msg.addr = 0x40;
    msg.flags = 0; // write
    msg.len = 1;
    msg.buf = buf;
    msg.buf[0] = CMD_TRIGGER_HUMI_NOHOLD; /* trigger humidity without hold I2C bus */

    sht2x_data.nmsgs = 1;
    sht2x_data.msgs = &msg;

    if (ioctl(fd, I2C_RDWR, &sht2x_data) < 0)
    {
        printf("%s() ioctl failure: %s\n", __func__, strerror(errno));
        return -2;
    }

    msleep(29);

    memset(buf, 0, sizeof(buf));
    msg.addr = 0x40;
    msg.flags = I2C_M_RD; // write
    msg.len = 3;
    msg.buf = buf;

    sht2x_data.nmsgs = 1;
    sht2x_data.msgs = &msg;

    if (ioctl(fd, I2C_RDWR, &sht2x_data) < 0)
    {
        printf("%s() ioctl failure: %s\n", __func__, strerror(errno));
        return -2;
    }
    // dump_buf("Relative humidity sample data: ", buf, 3);

    *rh = 125 * (((((int)buf[0]) << 8) + buf[1]) / 65536.0) - 6;
    return 0;
}

/**
 * @name: int sht2x_get_serialNumber(int fd, uint8_t *serialNumber, int size)
 * @description: SHT20获取序列号函数
 * @param {int} fd  文件描述符
 * @param {uint8_t} *serialNumber 序列号参数
 * @param {int} size 序列号长度
 * @return {*}
 */
int sht2x_get_serialNumber(int fd, uint8_t *serialnumber, int size)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data sht2x_data;
    uint8_t sbuf[2];
    uint8_t rbuf[4];

    if (fd < 0)
    {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__);
        return -1;
    }

    /*+------------------------------------------+
     *| Read SerialNumber from Location 1 |
     *+------------------------------------------+*/

    msgs[0].addr = 0x40;
    msgs[0].flags = 0; // write
    msgs[0].len = 2;
    msgs[0].buf = sbuf;

    msgs[0].buf[0] = 0xfa; /* command for readout on-chip memory */
    msgs[0].buf[1] = 0x0f; /* on-chip memory address */

    msgs[1].addr = 0x40;
    msgs[1].flags = I2C_M_RD; // write
    msgs[1].len = 4;
    msgs[1].buf = rbuf;

    sht2x_data.nmsgs = 2;
    sht2x_data.msgs = msgs;

    if (ioctl(fd, I2C_RDWR, &sht2x_data) < 0)
    {
        printf("%s() ioctl failure: %s\n", __func__, strerror(errno));
        return -2;
    }

    serialnumber[5] = rbuf[0]; /* Read SNB_3 */
    serialnumber[4] = rbuf[1]; /* Read SNB_2 */
    serialnumber[3] = rbuf[2]; /* Read SNB_1 */
    serialnumber[2] = rbuf[3]; /* Read SNB_0 */

    /*+------------------------------------------+
     *| Read SerialNumber from Location 2 |
     *+------------------------------------------+*/

    msgs[0].addr = 0x40;
    msgs[0].flags = 0; // write
    msgs[0].len = 2;
    msgs[0].buf = sbuf;

    msgs[0].buf[0] = 0xfc; /* command for readout on-chip memory */
    msgs[0].buf[1] = 0xc9; /* on-chip memory address */

    msgs[1].addr = 0x40;
    msgs[1].flags = I2C_M_RD; // write
    msgs[1].len = 4;
    msgs[1].buf = rbuf;

    sht2x_data.nmsgs = 2;
    sht2x_data.msgs = msgs;

    if (ioctl(fd, I2C_RDWR, &sht2x_data) < 0)
    {
        printf("%s() ioctl failure: %s\n", __func__, strerror(errno));
        return -2;
    }

    serialnumber[1] = rbuf[0]; /* Read SNC_1 */
    serialnumber[0] = rbuf[1]; /* Read SNC_0 */
    serialnumber[7] = rbuf[2]; /* Read SNA_1 */
    serialnumber[6] = rbuf[3]; /* Read SNA_0 */

    dump_buf("SHT2x Serial number: ", serialnumber, 8);
    return 0;
}