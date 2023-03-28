#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const char *chipname = "gpiochip0";
    struct gpiod_chip *chip;
    struct gpiod_line *lineRed;    // Red LED
    struct gpiod_line *lineGreed;  // Greed LED
    struct gpiod_line *lineYellow; // Yello LED
    struct gpiod_line *lineButton; // PushButton
    int i, val;

    // Open GPIO chip
    chip = gpiod_chip_open_by_name(chipname);

    // Open GPIO lines
    lineRed = gpiod_chip_get_line(chip, 20);   // GPIO20
    lineGreed = gpiod_chip_get_line(chip, 21); // GPIO21
    lineYellow = gpiod_chip_get_line(chip, 16); // GPIO16
    lineButton = gpiod_chip_get_line(chip, 6); // GPIO6

    // Open switch line for output
    gpiod_line_request_output(lineRed, "noah", 0);
    gpiod_line_request_output(lineYellow, "noah", 0);
    gpiod_line_request_output(lineGreed, "noah", 0);

    // Open switch line for input
    gpiod_line_request_input(lineButton, "noah");

    // 以二进制闪烁LED
    i = 0;
    while (1)
    {
        gpiod_line_set_value(lineRed, (i & 1) != 0);
        gpiod_line_set_value(lineGreed, (i & 2) != 0);
        gpiod_line_set_value(lineYellow, (i & 4) != 0);

        // Read button status and exit if pressed
        val = gpiod_line_get_value(lineButton);
        if (val == 0)
        {
            break;
        }

        usleep(100000); //100K us = 100ms
        i++;
    }

    // 释放线和芯片
    gpiod_line_release(lineRed);
    gpiod_line_release(lineGreed);
    gpiod_line_release(lineYellow);
    gpiod_line_release(lineButton);
    gpiod_chip_close(chip);
    return 0;
}