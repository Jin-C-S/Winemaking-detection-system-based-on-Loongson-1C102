#ifndef __PID_H
#define __PID_H

#include "ls1x.h"
#include "Config.h"
#include "ls1x_gpio.h"
#include "ls1x_latimer.h"
#include "ls1c102_ptimer.h"
#include "ls1x_common.h"
#include "ls1x_exti.h"
#include "ls1x_latimer.h"
#include "ls1c102_touch.h"
#include "ls1x_string.h"
#include "Config.h"

int16_t myabs(int a);
void pwm_control();
uint16_t pid1(int16_t speed1,uint16_t tar1);

#endif
