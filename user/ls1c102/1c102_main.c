/******************************************************
 * 项目名称：智能酿酒监测系统（主机端）
 * 
 * 项目描述：主机接收从机节点的环境数据（温湿度、酒精浓度）
 *           控制风扇、加湿器、注水器等执行器
 *           通过ESP8266上传数据至云平台
 * 
 * 硬件配置：龙芯1C102开发板，ESP8266模块，继电器模块
 *           温湿度传感器，酒精传感器，4P小白线
 * 
 * 从机数据格式：设备ID(1B)|温度高(1B)|温度低(1B)|湿度高(1B)|湿度低(1B)|酒精高(1B)|酒精低(1B)|报警(1B)
 * 
 * 接线说明：ESP8266模块 -> UART0 (GPIO_Pin_06, GPIO_Pin_07)
 *           执行器控制 -> GPIO_Pin_16(风扇), GPIO_Pin_17(加湿器), GPIO_Pin_18(注水器)
 ******************************************************/
#include "ls1x.h"
#include "Config.h"
#include "ls1x_gpio.h"
#include "ls1x_latimer.h"
#include "esp8266.h"
#include "ZigBee.h"
#include "ls1c102_interrupt.h"
#include "iic.h"
#include "UserGpio.h"
#include "oled.h"
#include "dht11.h"
#include "BEEP.h"
#include "led.h"
#include "queue.h"
#include "ls1x_clock.h"
#include "ls1c102_ptimer.h"
#include "FAN.h"

// 执行器控制引脚定义
//#define FAN_PIN        GPIO_PIN_16
#define HUMIDIFIER_PIN GPIO_PIN_17
#define WARM_PIN GPIO_PIN_34

// 环境阈值定义 (基于从机代码)
#define TEMP_HIGH_THRESHOLD    30   // 高温阈值(℃)
#define TEMP_Low_THRESHOLD    20   // 低温阈值(℃)
#define HUMI_LOW_THRESHOLD     40   // 低湿度阈值(%)
#define ALCOHOL_HIGH_THRESHOLD 500  // 酒精浓度报警阈值(2.50 mg/L)

// 修复1: 定义GPIO方向常量 (如果头文件中没有定义)
#ifndef GPIO_DIRECTION_OUTPUT
#define GPIO_DIRECTION_OUTPUT 1
#endif

void Uart2_init(uint32_t baud);

char str[50];
static uint16_t temp;         // 温度 (原始值)
static uint16_t humi;         // 湿度 (原始值)
static uint16_t alcohol;      // 酒精浓度 (放大100倍)
static uint8_t alarm_state;   // 报警状态
static uint8_t fan_state;     // 风扇状态
static uint8_t humidifier_state; // 加湿器状态
static uint8_t warm_state;    // 加热器状态

// 云平台数据包格式: 
// 头|温度(℃)|湿度(%)|酒精(浓度值)|报警|风扇|加湿|注水|校验和|尾
uint8_t data[11] = {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBB};

uint8_t data1[6] = {0x33, 0x31, 0x00, 0x34, 0x35, 0x36};//温度
uint8_t data2[6] = {0x33, 0x32, 0x00, 0x34, 0x35, 0x36};//湿度
uint8_t data3[6] = {0x33, 0x33, 0x00, 0x34, 0x35, 0x36};//酒精
uint8_t data4[6] = {0x33, 0x34, 0x00, 0x34, 0x35, 0x36};//风扇
uint8_t data5[6] = {0x33, 0x35, 0x00, 0x34, 0x35, 0x36};//加湿器
uint8_t data6[6] = {0x33, 0x36, 0x00, 0x34, 0x35, 0x36};//加热器

uint8_t Read_Buffer[255];     // 接收缓冲区
uint8_t Read_length;

 // 显示初始化标志
    static uint8_t display_initialized = 0;

    // 初始化显示界面
    void init_display() 
    {
    OLED_Clear();
    // 固定标签位置（不刷新）
    OLED_Show_Str(0, 0, "Sta:", 16);
    OLED_Show_Str(0, 2, "Temp: ", 16);
    OLED_Show_Str(0, 4, "Humi: ", 16);
    OLED_Show_Str(0, 6, "Alco: ", 16);
    display_initialized = 1;
    }

/// @brief 
/// @param arg 
/// @param args 
/// @return 
int main(int arg, char *args[])
{
    // 系统初始化
    SystemClockInit();        // 系统时钟配置
    GPIOInit();               // GPIO初始化
    OLED_Init();              // OLED显示屏初始化
    Uart1_init(9600);         // ESP8266通信串口初始化
    timer_init(5);            // 定时器初始化
    BEEP_Init();              // 蜂鸣器初始化
    EnableInt();              // 开启总中断
    FAN_Init();   // 风扇初始化
    Uart2_init(9600);

    // 执行器控制引脚初始化（取消注释）
    gpio_set_direction(HUMIDIFIER_PIN, GPIO_DIRECTION_OUTPUT);
    gpio_set_direction(WARM_PIN, GPIO_DIRECTION_OUTPUT);
   
    // 初始化执行器状态为关闭
    gpio_write_pin(HUMIDIFIER_PIN, 0);
    gpio_write_pin(WARM_PIN, 0);

    // ZigBee初始化（从机节点通信）
    DL_LN3X_Init(DL_LN3X_HOST, CHANNEL, Network1_Id);
    
    // 初始化数据队列
    Queue_Init(&Circular_queue);

    // 初始化显示
    init_display();
    delay_ms(1000);

    while (1)
    {  
        // 1. OLED显示更新
        // 温度显示（格式：xx.x）
        if(temp >=0) {
           OLED_Show_Str(48, 2, "      ", 16); // 清除旧内容
            sprintf(str, "%2d.%d℃ ", temp/10, temp%10);
            OLED_Show_Str(48, 2, str, 16); // 在固定位置显示
        }
        
        // 湿度显示（格式：xx.x）
        if(humi >=0) {
            OLED_Show_Str(48, 4, "      ", 16); // 清除旧内容
            sprintf(str, "%2d.%d%%RH", humi/10, humi%10);
            OLED_Show_Str(48, 4, str, 16);
        }
        
      // 酒精浓度显示（格式：xx.xx）
        if(alcohol >= 0) {
            OLED_Show_Str(48, 6, "      ", 16); // 清除旧内容
            uint16_t alc_integer = alcohol / 100;
            uint16_t alc_fraction = alcohol % 100;
            sprintf(str, "%2d.%02dmg/L ", alc_integer, alc_fraction);
            OLED_Show_Str(48, 6, str, 16);
        }

        // 执行器状态显示（在顶部显示）
        sprintf(str, "F:%d H:%d W:%d", 
                fan_state, humidifier_state, warm_state, alarm_state);
                OLED_Show_Str(30, 0, "            ", 16); // 清除旧状态
                OLED_Show_Str(30, 0, str, 16);
        
       
        // 2. 从ZigBee接收数据（使用队列方式）       
        if (Queue_isEmpty(&Circular_queue) == 0) 
        {
            Read_length = Queue_HadUse(&Circular_queue);
            Queue_Read(&Circular_queue, Read_Buffer, Read_length);
            
            // 解析数据包 - 根据实际数据包格式
            // 数据包格式: FE 0D 90 91 02 00 02 01 05 01 9A 00 DD 00 00 FF
            if (Read_length >= 16 && 
                Read_Buffer[0] == 0xFE && 
                Read_Buffer[1] == 0x0D && 
                Read_Buffer[15] == 0xFF) 
            {
                 // 解析传感器数据
                temp = (Read_Buffer[7] << 8) | Read_Buffer[8];    // 温度
                humi = (Read_Buffer[9] << 8) | Read_Buffer[10];   // 湿度
                alcohol = (Read_Buffer[11] << 8) | Read_Buffer[12]; // 酒精
                alarm_state = Read_Buffer[15];                     // 报警状态
                memset(Read_Buffer, 0, 255);
            }
        }  
                // 3. 执行器控制逻辑
                // 温度控制 - 高温开风扇
                if(temp/10 > TEMP_HIGH_THRESHOLD) {
                  
                   FAN_ON;
                    fan_state = 1;
                   // 报警提示
                    BEEP_ON;
                    delay_ms(200);
                    BEEP_OFF;
                } 
                //温度适中关风扇（设置回差2℃）
                else if(temp/10 < TEMP_HIGH_THRESHOLD - 2) {
                   
                   FAN_OFF;
                    fan_state = 0;
                } 

                // 低温加热
                if(temp/10 < TEMP_Low_THRESHOLD) {
                    gpio_write_pin(WARM_PIN, 1);
                    warm_state = 1;
                    // 报警提示
                    BEEP_ON;
                    delay_ms(200);
                    BEEP_OFF;
                }
                //加热到低温阈值+5处，关闭加热器
                else if(temp/10 > TEMP_Low_THRESHOLD + 5) {
                    gpio_write_pin(WARM_PIN, 0);
                    warm_state = 0;
                } 
                // 湿度控制 - 低湿度开加湿器
                if(humi/10 < HUMI_LOW_THRESHOLD) {
                    gpio_write_pin(HUMIDIFIER_PIN, 1);
                    humidifier_state = 1;
                    // 报警提示
                    BEEP_ON;
                    delay_ms(200);
                    BEEP_OFF;
                } 
                // 湿度适中关加湿器（设置回差5%）
                else if(humi/10 > HUMI_LOW_THRESHOLD + 5) {
                    gpio_write_pin(HUMIDIFIER_PIN, 0);
                    humidifier_state = 0;
                }
                
                // 酒精浓度控制 - 高浓度开启风扇 或 报警状态
                if(alcohol >= ALCOHOL_HIGH_THRESHOLD || alarm_state == 1) {
                   
                    FAN_ON;
                    fan_state = 1;
                    // 报警提示
                    BEEP_ON;
                    delay_ms(200);
                    BEEP_OFF;
                } 
                // 酒精浓度恢复正常关风扇
                else if(alcohol < ALCOHOL_HIGH_THRESHOLD - 20 ) {

                    FAN_OFF;
                    fan_state = 0;
                }
        
        //串口屏数据发送
        data1[2]=temp/10;
        data2[2]=humi / 10;
        data3[2]=alcohol;
        data4[2]=fan_state;
        data5[2]=humidifier_state;
        data6[2]=warm_state;

        UART_SendDataALL(UART2, data1, 6);
        UART_SendDataALL(UART2, data2, 6);
        UART_SendDataALL(UART2, data3, 6);
        UART_SendDataALL(UART2, data4, 6);
        UART_SendDataALL(UART2, data5, 6);
        UART_SendDataALL(UART2, data6, 6);


        // 4. 数据打包上传
        data[2] = temp / 10;      // 温度(整数部分)
        data[3] = humi / 10;      // 湿度(整数部分)
        data[4] = (alcohol >> 8) & 0xFF; // 酒精浓度高字节
        data[5] = alcohol & 0xFF;       // 酒精浓度低字节
        data[6] = alarm_state;    // 报警状态
        data[7] = fan_state;      // 风扇状态
        data[8] = humidifier_state; // 加湿器状态
        data[9] = warm_state;     // 加热器状态
        
        // 计算校验和 (所有数据字节之和取模256)
        uint8_t checksum = 0;
        for(int i = 2; i <= 9; i++) {
            checksum += data[i];
        }
        data[10] = checksum % 256;
        
        // 5. 通过ESP8266上传数据到云平台
        UART_SendDataALL(UART1, data, 11);
        
        // 6. 系统延时
        delay_ms(500);
    }
    
    return 0;
    
} 



 
               
                
