#include "key.h"
#include "ls1x_gpio.h"

void KEY_Init(void)
{
    gpio_set_direction(KEY1_PIN, GPIO_Mode_In);
    gpio_set_direction(KEY2_PIN, GPIO_Mode_In);
    gpio_set_direction(KEY3_PIN, GPIO_Mode_In);
    gpio_set_direction(KEY4_PIN, GPIO_Mode_In);
}

uint8_t KEY_Check(void)
{
    if (!gpio_get_pin(KEY1_PIN))// 读取按键状态
    { 
        return 1;
    }
    else if(!gpio_get_pin(KEY2_PIN))
    {
        return 2;
    }
    else if(!gpio_get_pin(KEY3_PIN))
    {
        return 3;
    }
    else if(!gpio_get_pin(KEY4_PIN))
    {
        return 4;
    }
    else
    {
        return 0;
    }
}
