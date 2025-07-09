#ifndef __ZIGBEE_H
#define __ZIGBEE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ls1x.h"

//节点ID：0x0001 0x0002 0x0003 0x0004
//主从机：主机     从机   从机   从机
#define ZIGBEE_RX_NODE 0X0001
#define ZIGBEE_TX1_NODE 0X0002

#define DL_LN3X_NODE 0x0001   //本机ID 
//信道
#define CHANNEL 0x13
//网络ID
#define Network1_Id 0x0003

// 添加主机/从机模式定义
#define DL_LN3X_HOST  0x0001   // 主机模式
#define DL_LN3X_SLAVE 0x0002   // 从机模式

typedef struct {
    uint8_t Data_Head_H;
	uint8_t Data_Head_L;
    uint8_t Noid_High;
    uint8_t Noid_Low;
    uint8_t Data_Mode;
    uint8_t Data_Len;
    uint8_t Data_Check;
    uint8_t Data_End;
} Send_Data_TypeDef;

uint8_t DL_LN3X_Init(uint16_t address, uint16_t channel, uint16_t Network_Id);

void DL_LN3X_Reset(void);
void DL_LN3X_Baud_Rate(uint16_t Baud_Rate);
void DL_LN3X_Network_Id(uint16_t Network_Id);
void DL_LN3X_Channel(uint16_t channel);
void DL_LN3X_Address(uint16_t address);
void DL_LN3X_Send(uint8_t *buf,uint8_t length, uint16_t address);
void Send_ZigBee_data(uint8_t Noid_id, uint16_t *data,uint8_t data_len, uint16_t address);

/*
// 添加解封装函数
void unpack_zigbee_data(uint8_t *raw_data, uint8_t length, 
                        uint16_t *temp, uint16_t *humi, 
                        uint16_t *alcohol, uint8_t *alarm);
*/

// 添加接收函数声明
int DL_LN3X_Receive(uint8_t *buf, uint8_t length);


#ifdef __cplusplus
}
#endif

#endif