#include "led.h"
#include "ls1x_gpio.h"


void LED_Init(void)
{
    // 设置LED1_PIN为输出模式
    gpio_set_direction(LED1_PIN, GPIO_Mode_Out);
    // 设置LED2_PIN为输出模式
    gpio_set_direction(LED2_PIN, GPIO_Mode_Out);
    // 设置LED3_PIN为输出模式
    gpio_set_direction(LED3_PIN, GPIO_Mode_Out);
    // 设置LED4_PIN为输出模式
    gpio_set_direction(LED4_PIN, GPIO_Mode_Out);
}

void LED_On(int LED_num)
{
    gpio_write_pin(LED_num, ON);
}

void LED_Off(int LED_num)
{
    gpio_write_pin(LED_num, OFF);
}

void LED_Toggle(int LED_num)
{
    gpio_write_pin(LED_num, ~LED_num);

}