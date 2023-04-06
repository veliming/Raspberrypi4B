/*
 * @Author: RoxyKko
 * @Date: 2023-03-26 11:22:00
 * @LastEditors: RoxyKko
 * @LastEditTime: 2023-04-06 17:27:03
 * @Description: iot项目-温湿度检测
 */
#include "iot_main.h"

#define I2C_BUS         "/dev/i2c-1"          // 设备总线地址
#define SHT20_ADDR      0x40                  // i2c设备物理地址
#define Vision          1.2                   // 版本号
#define lastEdit        "2023-04-06 16:12:59" // 最后编辑时间
#define TABLE_NAME      "RPI4B"               // 数据库表名
#define DATABASE_NAME   "sht20"            // 数据库名



static inline void print_usage(char *progname);
static inline void print_vision(char *progname);


int main(int argc, char **argv)
{
    int         opt;                                  // 命令行选项
    int         i2c_fd            =  -1;              // i2c设备文件描述符
    int         daemon_run        =   0;              // 后台运行标志
    char       *progname          =   NULL;           // 程序名
    int         error             =  -1;              // 报错提示符
    float       temp, rh          =   0;              // 温度、湿度
    char       *servip            =   NULL;           // 服务器ip
    int         port              =   0;              // 链接端口号
    char        buf[1024];                            // 数据暂存区
    int         socket_fd         =  -1;              // socket描述符
    bool        socket_connected  =   false;          // socket链接状态指示符
    packinfo_t  packinfo;                             // 数据包结构体
    int	        interval          =   4;              // 采样间隔，默认设为4s
    sqlite3     *db;                                   // 数据库句柄
    static double 	current_time  =   0;              // 当前时间
	static double 	latest_time   =   0;              // 上一次时间

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

    // 获取程序名
    progname = argv[0];
    if( logger_init("./logger/running.log", LOG_LEVEL_INFO) < 0)
    {
        fprintf(stderr, "logger system init failed!\n");
        return -1;
    }

    log_info("\n\n\n");
    log_info("============================================================\n");
    log_info("=                                                          =\n");
    log_info("=                      IOT Project                         =\n");
    log_info("=                                                          =\n");
    log_info("=                      Version: %.2f                        =\n", Vision);
    log_info("=                                                          =\n");
    log_info("=                      LastEdit: %s                       =\n", lastEdit);
    log_info("=                                                          =\n");
    log_info("============================================================\n");


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
            log_error("Invalid argument\n");
            break;
        }
    }

    // 检查IP和端口号
    if (!servip || !port)
    {
        print_usage(argv[0]);
        return 0;
    }

    // 后台运行
    if (daemon_run)
    {
        log_info("daemon run!\n");
        daemon(0, 0);
    }

    // 初始化sht20
    i2c_fd = sht2x_init();
    if (i2c_fd < 0)
    {
        log_error("sht2x initialize failed!\n");
        printf("sht2x initialize failed!\n");
        return -1;
    }
    log_info("sht2x initialize success!\n");

    // sht20软件复位
    if (sht2x_softReset(i2c_fd) < 0)
    {
        log_error("sht2x softReset failed!\n");
        printf("sht2x softReset failed!\n");
        return -2;
    }
    log_info("sht2x softReset success!\n");

    // 安装信号处理函数
    signal(SIGINT, SIG_IGN);

    // 初始化数据库
    if(database_init(DATABASE_NAME, &db) < 0)
    {
        log_error("database init failed!\n");
        printf("database init failed!\n");
        return -3;
    }

    // 若数据库中不存在表，则创建表
    if(database_create_table(TABLE_NAME, &db) < 0)
    {
        database_close(DATABASE_NAME, &db);
        return -4;
    }

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

    log_error("A required parameter is missing, the program exits");
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
    printf(" %s 该程序的版本为： %.2f 最后更新日期为： %s\n", progname, Vision, lastEdit);
    log_debug("Print vision success, the program exits");
    return;
}
