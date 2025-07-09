#include "ZigBee.h"
#include "Config.h"
#include "ls1x_string.h"
#include "ls1x_printf.h"
#include "ls1x_latimer.h"
#include "queue.h"
#include "ls1x_uart.h"

uint8_t dlln3x_address[10] = {0xFE, 0x07, 0x90, 0x21, 0x00, 0x00, 0x11, 0x90, 0x00, 0xFF};    // 设置设备地址
uint8_t dlln3x_network_id[10] = {0xFE, 0x07, 0x90, 0x21, 0x00, 0x00, 0x12, 0x91, 0x01, 0xFF}; // 设置网络ID
uint8_t dlln3x_channel[9] = {0xFE, 0x06, 0x90, 0x21, 0x00, 0x00, 0x13, 0x13, 0xFF};           // 设置信道
uint8_t dlln3x_baud[9] = {0xFE, 0x06, 0x90, 0x21, 0x00, 0x00, 0x14, 0x08, 0xFF};              // 设置波特率
uint8_t dlln3x_rest[8] = {0xFE, 0x05, 0x90, 0x21, 0x00, 0x00, 0x10, 0xFF};                    // 复位


/**************************************************************
功  能：ZigBee模块的初始化
参  数： address: 地址（必须不同） channel：信道（必须相同） Network_Id：网络ID（必须相同）
返回值： 无
**************************************************************/
uint8_t DL_LN3X_Init(uint16_t address, uint16_t channel, uint16_t Network_Id)
{
    Uart0_init(115200);             
    //将串口初始化为115200波特率，目的是在不知道zigbee默认的波特率为多少时，先通过115200的串口发送配置指令，将zigbee模块的波特率设置为9600
    DL_LN3X_Baud_Rate(0x02);        // 设置波特率为9600
    delay_ms(200);
    Uart0_init(9600);
    DL_LN3X_Baud_Rate(0x02);        // 设置波特率为9600

    delay_ms(200);
    delay_ms(200);
    DL_LN3X_Address(address);       // 设置地址0x0001
    delay_ms(200);
    DL_LN3X_Channel(channel);       // 设置信道0x0B
    delay_ms(200);
    DL_LN3X_Network_Id(Network_Id); // 设置网络ID
    delay_ms(200);

    Uart0_init(115200);
    //将串口初始化为115200波特率，目的是确保zigbee模块成功复位，成功复位时，zigbee模块的指示灯全部亮起
    DL_LN3X_Reset();
    delay_ms(200);
    Uart0_init(9600);
    DL_LN3X_Reset();
}

/**************************************************************
功  能： ZigBee模块地址配置
参  数： address: 地址取值范围为 0x0001~0xfffe，
                同一个网络中的模块地址必须不相同。
返回值： 无
**************************************************************/
void DL_LN3X_Address(uint16_t address)
{
    uint8_t temp = 0;
    temp = address;
    dlln3x_address[7] = temp & 0x00ff;
    dlln3x_address[8] = temp >> 8;
    UART_SendDataALL(UART0, dlln3x_address, 10);
}

/**************************************************************
功  能： ZigBee模块信道配置
参  数： channel:  取值范围为 0x0B~0x1A，同一个网络中的模块信道必须相同。
返回值： 无
**************************************************************/
void DL_LN3X_Channel(uint16_t channel)
{
    uint8_t temp = 0;
    temp = channel;
    dlln3x_channel[7] = temp;
    UART_SendDataALL(UART0, dlln3x_channel, 9);
}

/**************************************************************
功  能： ZigBee模块网络ID配置
参  数： 取值范围为 0x0001~0xfffe，同一个网络中的模块网络ID必须相同。
返回值： 无
**************************************************************/
void DL_LN3X_Network_Id(uint16_t Network_Id)
{
    uint8_t temp = 0;
    temp = Network_Id;
    dlln3x_network_id[7] = temp & 0x00ff;
    dlln3x_network_id[8] = temp >> 8;
    UART_SendDataALL(UART0, dlln3x_network_id, 10);
}

/**************************************************************
功  能： ZigBee模块波特率配置
参  数： Baud_Rate  波特率对照表:
                波特率   读数	 波特率   读数
                2400    0x00    57600  	0x07
                4800	0x01	115200	0x08
                9600	0x02	230400	0x09
                14400	0x03	125000	0x0A
                19200   0x04	250000	0x0B
                28800	0x05	500000	0x0C
                38400	0x06

返回值： 无
**************************************************************/
void DL_LN3X_Baud_Rate(uint16_t Baud_Rate)
{
    uint8_t temp = 0;
    temp = Baud_Rate;
    dlln3x_baud[7] = temp;
    UART_SendDataALL(UART0, dlln3x_baud, 9);
}

/**************************************************************
功  能： ZigBee模块网络ID配置
参  数： 配置完成后需重启模块
返回值： 无
**************************************************************/
void DL_LN3X_Reset(void)
{
    UART_SendDataALL(UART0, dlln3x_rest, 8);
}

/**************************************************************
功  能： ZigBee模块发送函数
参  数： 配置完成后需重启模块
返回值： 无
**************************************************************/
void DL_LN3X_Send(uint8_t *buf, uint8_t length, uint16_t address)
{
    uint8_t i;

    UART_SendData(UART0, 0xFE);
    UART_SendData(UART0, length + 4);
    UART_SendData(UART0, 0x90); // 源端口
    UART_SendData(UART0, 0x91); // 目标端口

    UART_SendData(UART0, address & 0x00ff); // 发送远程地址小端模式，即先传输低 8 位，再传输高 8 位。
    UART_SendData(UART0, address >> 8);

    for (i = 0; i < length; i++)
    {
        if (buf[i] == 0xff)
        {
            UART_SendData(UART0, 0xFE);
            UART_SendData(UART0, 0xFD);
        }
        else if (buf[i] == 0xfe)
        {
            UART_SendData(UART0, 0xFE);
            UART_SendData(UART0, 0xFC);
        }
        else
        {
            UART_SendData(UART0, buf[i]);
        }
    }
    UART_SendData(UART0, 0xFF);
}


int DL_LN3X_Receive(uint8_t *buf, uint8_t length)
{
    uint8_t received = 0;
    uint8_t data_byte;
    uint8_t frame_started = 0;
    uint8_t escape_next = 0;
    
    while (UART_GetDataCount(UART0) > 0 && received < length)
    {
        data_byte = UART_ReceiveData(UART0);
        
        // 检查转义序列
        if (escape_next) {
            if (data_byte == 0xFC) {
                buf[received++] = 0xFE;
            } else if (data_byte == 0xFD) {
                buf[received++] = 0xFF;
            }
            escape_next = 0;
            continue;
        }
        
        // 检查帧头
        if (!frame_started) {
            if (data_byte == 0xFE) {
                frame_started = 1;
            }
            continue;
        }
        
        // 处理特殊字节
        if (data_byte == 0xFE) {
            escape_next = 1;
        } 
        // 帧结束符
        else if (data_byte == 0xFF) {
            break; // 结束帧接收
        }
        // 普通数据
        else {
            buf[received++] = data_byte;
        }
    }
    
    return received;
}



/*数据上行发送函数封装
 * Noid_id  发送端节点ID
 * data     数据指针
 * data_len 数据长度
 * address  接收端节点ID
 */
void Send_ZigBee_data(uint8_t Noid_id, uint16_t *data, uint8_t data_len, uint16_t address)
{
    uint8_t ZigBee_TX_Data[20];
    uint16_t sum = 0;
    int i = 0;

    Send_Data_TypeDef Data_TypeDef;
    Data_TypeDef.Data_Head_H = 0x55;
    Data_TypeDef.Data_Head_L = 0xAA;

    Data_TypeDef.Noid_High = Noid_id / 256;
    Data_TypeDef.Noid_Low = Noid_id % 256;
    Data_TypeDef.Data_Mode = 0x00;
    Data_TypeDef.Data_Len = data_len;

    ZigBee_TX_Data[0] = Data_TypeDef.Data_Head_H;
    ZigBee_TX_Data[1] = Data_TypeDef.Data_Head_L;
    ZigBee_TX_Data[2] = Data_TypeDef.Noid_High;
    ZigBee_TX_Data[3] = Data_TypeDef.Noid_Low;
    ZigBee_TX_Data[4] = Data_TypeDef.Data_Mode;
    ZigBee_TX_Data[5] = Data_TypeDef.Data_Len;

    data_len -= 0x30;
    for (i = 0; i < data_len; i++)
    {
        ZigBee_TX_Data[6 + i] = data[i];
    }
    for (i = 2; i < data_len + 4; i++)
    {
        sum += ZigBee_TX_Data[i];
    }

    


    Data_TypeDef.Data_Check = sum % 256;
    Data_TypeDef.Data_End = 0xbb;
    ZigBee_TX_Data[data_len + 6] = Data_TypeDef.Data_Check;
    ZigBee_TX_Data[data_len + 7] = Data_TypeDef.Data_End;

    DL_LN3X_Send(ZigBee_TX_Data, data_len + 8, address);

}