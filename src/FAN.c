#include "Fan.h"

extern int duty;

void FAN_Init(void)
{
    gpio_set_direction(FAN, GPIO_Mode_Out);
}


