#include "BEEP.h"
#include "ls1x_gpio.h"

void BEEP_Init(void)
{
    gpio_set_direction(BEEP, GPIO_Mode_Out);
}
