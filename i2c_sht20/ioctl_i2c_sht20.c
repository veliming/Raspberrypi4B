/*
 * @Author: RoxyKko
 * @Date: 2023-03-24 18:50:49
 * @LastEditors: RoxyKko
 * @LastEditTime: 2023-03-25 21:58:46
 * @Description: sht20驱动
 */
#include <ioctl_i2c_sht20.h>

int main(int argc, char **argv)
{
    int fd;
    float temp, rh;
    uint8_t serilaNumber[8];

    fd = sht2x_init();
    if (fd < 0)
    {
        printf("sht2x initialize failed!\n");
        return 1;
    }
    printf("sht2x initialize success!\n");

    if (sht2x_softReset(fd) < 0)
    {
        printf("sht2x softReset failed!\n");
        return 2;
    }
    printf("sht2x softReset success!\n");

    if (sht2x_get_serialNumber(fd, serilaNumber, sizeof(serilaNumber)) < 0)
    {
        printf("sht2x show serialNumber failed!\n");
        return 3;
    }
    printf("sht2x get serialNumber success!\n");

    if (sht2x_get_temp_humidity(fd, &temp, &rh) < 0)
    {
        printf("sht2x get temp and humidity failed!\n");
        return 4;
    }
    printf("sht2x get temp and humidity success!\n");

    printf("Temperature = %lf C, Humidity = %lf %%RH\n", temp, rh);
    close(fd);
    return 0;
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

#ifdef I2C_API_RDWR
/**
 * @name: int sht2x_init(void)
 * @description: 初始化sht20
 * @return fd 返回文件描述符
 */
int sht2x_init(void)
{
    int fd;
    if ((fd = open(I2C_BUS, O_RDWR)) < 0)
    {
        printf("i2c device open failed: %s\n", strerror(errno));
        return -1;
    }

    if (ioctl(fd, I2C_TENBIT, 0) != 0)
    {
        printf("i2c device ioctl setting 0-7bits failed: %s\n", strerror(errno));
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, SHT20_ADDR) < 0)
    {
        printf("i2c device ioctl SHT20_ADDR I2C_SLAVE failed: %s\n", strerror(errno));
        return -1;
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
    uint8_t buf[4];

    if (fd < 0)
    {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__);
        return -1;
    }

    /* 软件复位SHT2x */
    memset(buf, 0, sizeof(buf));

    buf[0] = CMD_SOFT_RESET;
    write(fd, buf, 1);

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
    uint8_t buf[4];

    if (fd < 0 || !temp || !rh)
    {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__);
        return -1;
    }

    /* 发送触发温度测量命令然后读取数据 */
    memset(buf, 0, sizeof(buf));
    buf[0] = CMD_TRIGGER_TEMP_NOHOLD;
    write(fd, buf, 1);

    msleep(85); // 数据表:typ=66, max=85

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 3);
    dump_buf("Temperaure sample data: ", buf, 3);
    // 温度计算公式 T= -46.85 + 175.72 * ST/2^16
    *temp = 175.72 * (((((int)buf[0]) << 8) + buf[1]) / 65536.0) - 46.85; // ST占两个字节，i2c先传高字节，再传低字节，buf[0]左移8位，再加上buf[1]，即为ST

    /* 发送触发湿度测量命令然后读取数据 */
    memset(buf, 0, sizeof(buf));
    buf[0] = CMD_TRIGGER_HUMI_NOHOLD;
    write(fd, buf, 1);

    msleep(29); // 数据表:typ=22, max=29

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 3);
    dump_buf("Humidity sample data: ", buf, 3);
    // 湿度计算公式 RH= -6 + 125 * SRH/2^16
    *rh = 125 * (((((int)buf[0]) << 8) + buf[1]) / 65536.0) - 6; // SRH占两个字节，i2c先传高字节，再传低字节，buf[0]左移8位，再加上buf[1]，即为SRH

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
int sht2x_get_serialNumber(int fd, uint8_t *serialNumber, int size)
{
    uint8_t buf[4];

    if (fd < 0 || !serialNumber || size != 8)
    {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__);
        return -1;
    }

    /* 从区域1读取序列号 */
    memset(buf, 0, sizeof(buf));
    buf[0] = 0xfa; // 读出片上内存命令
    buf[1] = 0x0f; // 读出片上内存地址
    write(fd, buf, 2);

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 4);

    serialNumber[5] = buf[0]; /* Read SNB_3 */
    serialNumber[4] = buf[1]; /* Read SNB_2 */
    serialNumber[3] = buf[2]; /* Read SNB_1 */
    serialNumber[2] = buf[3]; /* Read SNB_0 */

    /* 从区域2读取序列号 */
    memset(buf, 0, sizeof(buf));
    buf[0] = 0xfc; // 读出片上内存命令
    buf[1] = 0xc9; // 读出片上内存地址
    write(fd, buf, 2);

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 4);

    serialNumber[1] = buf[0]; /* Read SNC_1 */
    serialNumber[0] = buf[1]; /* Read SNC_0 */
    serialNumber[7] = buf[2]; /* Read SNA_1 */
    serialNumber[6] = buf[3]; /* Read SNA_0 */

    dump_buf("Serial Number: ", serialNumber, 8);

    return 0;
}

#elif (defined I2C_API_IOCTL)

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
#endif // I2C_API_RDWR
