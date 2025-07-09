#ifndef _FAN_H_
#define _FAN_H_

#include "ls1x.h"
#include "ls1x_gpio.h"   

#define FAN  GPIO_PIN_35
#define FAN_ON gpio_write_pin(FAN, 1)
#define FAN_OFF gpio_write_pin(FAN, 0)

void FAN_Init(void);

#endif
