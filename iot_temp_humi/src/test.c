#include "iot_main.h"
#include "get_time.h"

int main(int argc, char **argv)
{
    char datime[40];
    double time = get_time(datime);
    printf("%s\n", datime);
    return 0;
}