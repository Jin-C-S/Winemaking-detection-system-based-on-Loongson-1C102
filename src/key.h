#ifndef __KEY_H
#define __KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ls1x.h"


#define KEY1_PIN    GPIO_PIN_22
#define KEY2_PIN    GPIO_PIN_23
#define KEY3_PIN    GPIO_PIN_24
#define KEY4_PIN    GPIO_PIN_25

void KEY_Init(void);
uint8_t KEY_Check(void);


#ifdef __cplusplus
}
#endif

#endif // __KEY_H
