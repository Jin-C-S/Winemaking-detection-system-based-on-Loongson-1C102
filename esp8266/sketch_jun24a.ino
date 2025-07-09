//连接wifi后登陆MQTT，然后每1s上报一次数据(数据每次加1)
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <String.h>
#include <Ticker.h>
#include <stdio.h>


#define led 2  //发光二极管连接在8266的GPIO2上


#define WIFI_SSID "test"    //wifi名称
#define WIFI_PASSWORD "00000000"  //wifi密码
//MQTT三元组
#define ClientId "c30a4d9af405f3470a3a5034ac7ecd09"
#define Username "bkrc"
#define Password "88888888"

#define MQTT_Address "115.28.209.116"
#define MQTT_Port 1883
#define Iot_link_MQTT_Topic_Report "device/7cd2a1183bb6bc36/up"
#define Iot_link_MQTT_Topic_Commands "device/7cd2a1183bb6bc36/down"

WiFiClient myesp8266Client;
PubSubClient client(myesp8266Client);

// 全局JSON缓冲区和消息缓冲区
StaticJsonBuffer<1024> jsonBuffer;
char JSONmessageBuffer[1024];

char printf_buf[16] = { 0 };
unsigned char buffer[32];
unsigned char network_buffer[5];
char total_data[50] = { 0 };

unsigned char rx_buffer[32];
uint16_t rx_length = 0;

int data_temp = 1;
int mqtt_state = 0;
int mqtt_sub_state = 1;

uint16_t Smoke_value = 0, bh1750_value = 0, balance = 0, door_flag = 0;




void setup() {
  pinMode(led, HIGH);
  Serial.begin(9600);
  WIFI_Init();
  MQTT_Init();
}

void loop() {
  client.loop();  // 保持MQTT服务器连接
  
  // 处理串口数据
  if (Serial.available()) {
    char c = Serial.read();  // 读取一个字节
    
    if (rx_length == 0) {
      if (c == 0x55) {  // 检测包头
        rx_buffer[rx_length++] = c;
      }
    } 
    else {
      rx_buffer[rx_length++] = c;
      
      // 检查包尾或最大长度
      if (c == 0xBB || rx_length >= 11) {
        // 完整数据包接收完毕，进行处理
        if (rx_length >= 2) {  // 确保至少有包头和包尾
          protocolJSON(rx_buffer);
        }
        rx_length = 0;  // 重置接收计数器
      }
    }
  }
  
  // 检查WiFi和MQTT连接状态
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi连接断开，正在重连...");
    WIFI_Init();
  } else if (!client.connected()) {
    Serial.println("MQTT连接断开，正在重连...");
    MQTT_Init();
  }
  
  delay(100);
}

/*

void loop() {
  
  client.loop();  // 保持MQTT服务器连接
  
 
 
  // 处理串口数据
 if (Serial.available()) {
  char c = Serial.read();  // 仅读取一次
  
  if (rx_length == 0) {
    if (c == 0x55) {
     rx_buffer[rx_length++] = c;
    }
  } 
  else {
    rx_buffer[rx_length++] = c;
    if (c == 0xBB) {
      rx_length = 0;
    } else if (rx_length >= 11) {  // 按11字节数据包限制
      rx_length = 0;
    }
  }
}
    // 将串口发来的数据进行转换JSON数据发布到云平台
    protocolJSON(rx_buffer);

    // 清空串口数据
    while (Serial.read() >= 0) {};






  // 检查WiFi和MQTT连接状态
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi连接断开，正在重连...");
    WIFI_Init();  // 调用重连函数
  } else {
    if (!client.connected()) {
      Serial.println("MQTT连接断开，正在重连...");
      MQTT_Init();  // 调用MQTT重连函数
    }
  }
  
  delay(100);  // 减少CPU占用
}


*/



void WIFI_Init() {
  // 重置LED状态为闪烁，指示正在连接
  digitalWrite(led, LOW);
  delay(200);
  digitalWrite(led, HIGH);
  delay(200);
  
  Serial.println("\n=== WiFi连接开始 ===");
  Serial.print("连接到SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);           // 设置为STA模式
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // 连接WiFi
  
  // 定义连接超时时间（20秒）
  const unsigned long timeout = 20000;
  unsigned long start = millis();
  
  // 等待连接或超时
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(led, !digitalRead(led));  // LED闪烁
    
    Serial.print(".");
    
    // 检查是否超时
    if (millis() - start > timeout) {
      Serial.println("\nWiFi连接超时!");
      break;
    }
  }
  
  // 检查连接结果
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(led, HIGH);       // 连接成功，LED常亮
    Serial.println("\nWiFi连接成功!");
    Serial.print("获取到的IP地址: ");
    Serial.println(WiFi.localIP());
    
    // 连接成功后初始化MQTT
    MQTT_Init();
  } else {
    digitalWrite(led, LOW);        // LED熄灭表示连接失败
    Serial.println("\nWiFi连接失败，5秒后重试...");
    delay(5000);
  }
}


// 连接MQTT（非阻塞方式）
void MQTT_Init() {
  client.setServer(MQTT_Address, MQTT_Port);
  client.setClient(myesp8266Client);
  client.subscribe(Iot_link_MQTT_Topic_Commands);
  client.setCallback(callback);
  
  // 非阻塞连接
  if (!client.connected()) {
    bool connected = client.connect(ClientId, Username, Password);
    if (connected) {
      Serial.println("MQTT连接成功");
      client.subscribe(Iot_link_MQTT_Topic_Commands);
    } else {
      Serial.print("MQTT连接失败，状态码: ");
      Serial.println(client.state());
      
      // 发送连接失败指示
      network_buffer[0] = 0x55;
      network_buffer[1] = 0xAA;
      network_buffer[2] = 0x02;
      network_buffer[3] = 0x00;
      network_buffer[4] = 0x00;
      network_buffer[5] = 0xBB;
      Serial.write(network_buffer, 6);
    }
  }
}


/* 云端下发 */
void callback(char* topic, byte* payload, unsigned int length) {
  mqtt_sub_state = 0;
  JsonObject& root = jsonBuffer.parseObject(payload);
  
  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  
  String sign = root["message"];
  JsonObject& root1 = jsonBuffer.parseObject(sign);
  String sign1 = root1["sign"];

  // 接收到，led灯闪烁一次
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);

  jsonBuffer.clear();
}

void protocolJSON(unsigned char JsonData[]) {
  static StaticJsonBuffer<1024> jsonBuffer;
  static int sendCounter = 0;  // 使用计数器轮流发送不同类型数据
  char JSONmessageBuffer[1024];
  
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.createObject();
  
  root["sign"] = "7cd2a1183bb6bc36";
  root["type"] = 1;

  JsonObject& data = root.createNestedObject("data");
  
  // 轮流发送温度、湿度和酒精浓度数据
  switch(sendCounter % 3) {
    case 0: {  // 发送温度数据
      JsonObject& tempObj = data.createNestedObject("Temp");
      tempObj["temperature"] = JsonData[2];
      break;
    }
      
    case 1: {  // 发送湿度数据
      JsonObject& humidObj = data.createNestedObject("Humid");
      humidObj["humidity"] = JsonData[3];
      break;
    }
      
    case 2: {  // 发送酒精浓度数据
      JsonObject& alcoholObj = data.createNestedObject("Alco");
      // 合并高字节(JsonData[4])和低字节(JsonData[5])组成完整的酒精浓度值
      uint16_t alcoholValue = (JsonData[4] << 8) | JsonData[5];
      alcoholObj["alco"] = alcoholValue;
      break;
    }
  }
  
  // 增加计数器，下次发送下一种数据
  sendCounter++;

  // 生成JSON字符串
  root.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  
  // 发布消息（确保client已连接）
  if (client.connected()) {
    client.publish(Iot_link_MQTT_Topic_Report, JSONmessageBuffer);
    Serial.println("Published: " + String(JSONmessageBuffer));
  } else {
    Serial.println("MQTT未连接，无法发布消息");
  }
  
  // 发送确认数据
  network_buffer[0] = 0x55;
  network_buffer[1] = 0xAA;
  network_buffer[2] = 0x03;
  network_buffer[3] = 0x00;
  network_buffer[4] = 0x00;
  network_buffer[5] = 0xBB;
  Serial.write(network_buffer, 6);
  
  // LED反馈
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
}


/*


void protocolJSON(unsigned char JsonData[]) {
  static StaticJsonBuffer<1024> jsonBuffer;
  static int sendCounter = 0;  // 使用计数器轮流发送不同类型数据
  char JSONmessageBuffer[1024];
  
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.createObject();
  
  root["sign"] = "7cd2a1183bb6bc36";
  root["type"] = 1;

  JsonObject& data = root.createNestedObject("data");
  
  // 轮流发送温度、湿度和酒精浓度数据
  switch(sendCounter % 3) {
    case 0: {  // 发送温度数据
      JsonObject& tempObj = data.createNestedObject("Temp");
      tempObj["temperature"] = JsonData[2];
      break;
    }
      
    case 1: {  // 发送湿度数据
      JsonObject& humidObj = data.createNestedObject("Humid");
      humidObj["humidity"] = JsonData[3];
      break;
    }
      
    case 2: {  // 发送酒精浓度数据
      JsonObject& alcoholObj = data.createNestedObject("Alco");
      alcoholObj["alco"] = JsonData[4];
      break;
    }
  }
  
  // 增加计数器，下次发送下一种数据
  sendCounter++;

  // 生成JSON字符串
  root.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  
  // 发布消息（确保client已连接）
  if (client.connected()) {
    client.publish(Iot_link_MQTT_Topic_Report, JSONmessageBuffer);
    Serial.println("Published: " + String(JSONmessageBuffer));
  } else {
    Serial.println("MQTT未连接，无法发布消息");
  }
  
  // 发送确认数据
  network_buffer[0] = 0x55;
  network_buffer[1] = 0xAA;
  network_buffer[2] = 0x03;
  network_buffer[3] = 0x00;
  network_buffer[4] = 0x00;
  network_buffer[5] = 0xBB;
  Serial.write(network_buffer, 6);
  
  // LED反馈
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
}

*/
