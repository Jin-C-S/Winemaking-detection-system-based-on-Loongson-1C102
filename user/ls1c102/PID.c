#include "ls1x.h"
#include "Config.h"
#include "ls1x_gpio.h"
#include "PID.h"

uint16_t Err=0,last_err=0,next_err=0,pwm=0,add=0,p=3,i=1,d=0;  

uint16_t Err2=0,last_err2=0,next_err2=0,pwm2=0,add2=0,p2=4,i2=0,d2=0;

int16_t myabs(int a)
{ 		   
	  int temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}

void pwm_control()
{
    if(pwm>99)
        pwm=99;
    if(pwm<0)
        pwm=0;
}

uint16_t pid1(int16_t speed1,uint16_t tar1)
{
    speed1=myabs(speed1);
    Err=tar1-speed1;           
    add=p*(Err-last_err)+i*(Err)+d*(Err+next_err-2*last_err);  
    pwm+=add;
    pwm_control();
    next_err=last_err;
    last_err=Err;
    return pwm;
}


