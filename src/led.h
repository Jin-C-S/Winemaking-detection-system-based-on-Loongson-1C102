#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ls1x.h"

#define ON      1
#define OFF     0

#define LED1_PIN    GPIO_PIN_20
#define LED2_PIN    GPIO_PIN_28
#define LED3_PIN    GPIO_PIN_27
#define LED4_PIN    GPIO_PIN_26

void led_init(void);
void LED_On(int LED_num);
void LED_Off(int LED_num);
void LED_Toggle(int LED_num);


#ifdef __cplusplus
}
#endif

#endif // ___LED_H
