#ifndef __BEEP_H
#define __BEEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ls1x.h"


#define BEEP        GPIO_PIN_63
#define BEEP_ON     gpio_write_pin(BEEP, 1)
#define BEEP_OFF    gpio_write_pin(BEEP, 0)

void BEEP_Init(void);


#ifdef __cplusplus
}
#endif

#endif // __KEY_H
