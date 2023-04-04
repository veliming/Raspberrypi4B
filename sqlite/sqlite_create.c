#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>

static int callback(void *para, int n_column, char **column_value, char **column_name)
{
    int  i;
    for(i = 0; i<n_column; i++)
    {
        printf("%-20s = %-20s \n", column_name[i], column_value[i] ? column_value[i] : "NULL");    // 打印数据表，若为空则打印NULL
    }
    printf("\n ");
    return 0;
}

int main(int argc, char **argv)
{
    sqlite3 *db;          // 数据表
    char *errmsg;         // 报错信息
    int rv;               // 返回值
    char *sql;
    const char* printData = "Callback function called";

    rv = sqlite3_open("test.db", &db);   // 打开数据表文件，若不存在则创建

    if(rv != SQLITE_OK)                  // 成功返回SQLITE_OK = 0，否则返回非0值
    {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    printf("Opened database successfully!\n");

    // 创建SQL语句
    // 创建名为‘COMPANY’的‘TABLE’表格
    // ID：这是一个 INT 类型的列 使用 PRIMARY KEY 限制条件作为表格的主键 NOT NULL 限制条件表示该列不能包含 NULL 值
    // NAME：这是一个 TEXT 类型的列 NOT NULL 限制条件表示该列不能包含 NULL 值
    // AGE：这是一个 INT 类型的列，可以存储整数值 NOT NULL 限制条件表示该列不能包含 NULL 值
    // ADDRESS：这是一个 CHAR(50) 类型的列，可以存储长度最多为 50 个字符的文本数据。没有NOT NULL因此可以包含 NULL 值
    // SALARY：这是一个 REAL 类型的列，可以存储浮点数值 没有NOT NULL因此可以包含 NULL 值
    sql = "CREATE TABLE COMPANY(" \
          "ID INT PRIMARY KEY    NOT NULL," \
          "NAME           TEXT   NOT NULL," \
          "AGE            INT    NOT NULL," \
          "ADDRESS        CHAR(50)," \
          "SALARY         REAL );";

    //
    rv = sqlite3_exec(db, sql, callback, 0, &errmsg);// 参数依次为 数据表，sql语句，回调函数，给回调函数的传参，错误信息
    if(rv != SQLITE_OK)
    {
        printf("SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
    }
    else
        printf("Table create successfully!\n");

    // 插入SQL语句
    // 插入到表格（列）值（对应列的值）
    sql = "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
          "VALUES (1, 'Paul', 32, 'California', 20000.00);" \
          "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
          "VALUES (2, 'Allen', 25, 'Texas', 15000.00);" \
          "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
          "VALUES (3, 'Teddy', 23, 'Norway', 20000.00);" \
          "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
          "VALUES (4, 'Mark', 25, 'Rich-Mond', 65000.00);" ;

    //完成SQL语句 
    rv = sqlite3_exec(db, sql, callback, 0, &errmsg);// 参数依次为 数据表，sql语句，回调函数，给回调函数的传参，错误信息
    if(rv != SQLITE_OK)
    {
        printf("SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    printf("Records create successfully!\n");

    // 查询SQL语句
    sql = "SELECT * from COMPANY";

    //完成SQL语句 
    rv = sqlite3_exec(db, sql, callback, (void*)printData, &errmsg);// 参数依次为 数据表，sql语句，回调函数，给回调函数的传参，错误信息
    if(rv != SQLITE_OK)
    {
        printf("SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    printf("Inquire Records successfully!\n");

    // 删除SQL语句
    sql = "DELETE FROM COMPANY WHERE ID = 4;";

    //完成SQL语句 
    rv = sqlite3_exec(db, sql, callback, (void*)printData, &errmsg);// 参数依次为 数据表，sql语句，回调函数，给回调函数的传参，错误信息
    if(rv != SQLITE_OK)
    {
        printf("SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    printf("Delete Records successfully!\n");

    // 查询SQL语句
    sql = "SELECT * from COMPANY";

    //完成SQL语句 
    rv = sqlite3_exec(db, sql, callback, (void*)printData, &errmsg);// 参数依次为 数据表，sql语句，回调函数，给回调函数的传参，错误信息
    if(rv != SQLITE_OK)
    {
        printf("SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    printf("Inquire Records successfully!\n");

    // 查询SQL最后一句
    sql = "SELECT * FROM COMPANY ORDER BY ID DESC LIMIT 1;";
    //完成SQL语句 
    rv = sqlite3_exec(db, sql, callback, (void*)printData, &errmsg);// 参数依次为 数据表，sql语句，回调函数，给回调函数的传参，错误信息
    if(rv != SQLITE_OK)
    {
        printf("SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    printf("Inquire Last Records successfully!\n");


    sqlite3_close(db);                  // 关闭数据表文件，返回值同sqlite_open
    return 0;
}
