#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h> // 使用 <time.h> 库来获取时间
#include "CalcFunction.h"
#include "TimeLibExternalFunction.h"

typedef struct {
  int pin;
  String name;
  double a;
  double b;
  double result;
} ph_meter_setting_t;

ph_meter_setting_t pH_meterSettingList[5]; 

#define NUM_READINGS 100

// WiFi网络的SSID和密码
const char* ssid = "IDWATER";
const char* password = "56651588";

// NTP 服务器地址
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800; // 东八区（GMT+8）
const int   daylightOffset_sec = 0; // 无夏令时偏移

// HTTP服务器的地址和端口
const char* serverAddress = "http://192.168.20.223:8080";
const char* endpoint = "/PH/data";

double readPH(ph_meter_setting_t ph_meter);
void connectToWiFi();


void setup() {
  Serial.begin(115200);  // 初始化串口通訊
  connectToWiFi(); // 尝试连接到WiFi网络
  SPI.begin(); // 初始化 SPI 介面
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  // 启动 WiFi 连接

  pH_meterSettingList[0] = {33, "pH0001", 9.9852, 3.7507};
  pH_meterSettingList[1] = {34, "pH0002", 15.659, 9.5989};
  pH_meterSettingList[2] = {34, "pH0003", 5.2545, 1.4127};
  pH_meterSettingList[3] = {32, "pH0004", 3.5039, 3.8348};
  pH_meterSettingList[4] = {39, "pH0005", -42.568, 11.541};

  while (!time(nullptr)) {
    delay(100);
    Serial.println("等待时间同步..."); // 等待时间同步完成
  }
}

void loop() {
  // 获取当前时间
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("获取本地时间失败");
    return;
  }

  // 格式化时间为指定格式
  char formattedTime[50];
  strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%dT%H:%M:%S", &timeinfo);


  for (int ph_index = 0; ph_index < 5; ph_index++) {
    uint16_t v_buffer[NUM_READINGS];
    for(int i = 0; i < NUM_READINGS; i++){
      v_buffer[i] = (uint16_t)readPH(pH_meterSettingList[ph_index]);
      delay(100);
    }
    pH_meterSettingList[ph_index].result = afterFilterValue(v_buffer,NUM_READINGS);
    String DataTime = GetDatetimeString("-","T",":");
    Serial.printf("%s, %s, %.2f\n", 
      pH_meterSettingList[ph_index].name.c_str(),
      DataTime.c_str(),
      pH_meterSettingList[ph_index].result
    );
    HTTPClient http1;
    String url1 = String(serverAddress) + String(endpoint) + 
                  "?name=" + pH_meterSettingList[ph_index].name + 
                  "&time=" + DataTime + 
                  "&value=" + String(pH_meterSettingList[ph_index].result, 2);
    http1.begin(url1); // 开始第一个HTTP连接
    int httpResponseCode1 = http1.GET(); // 发送第一个GET请求
    if (httpResponseCode1 > 0) {
      Serial.print("HTTP Response code 1: ");
      Serial.println(httpResponseCode1);
    } else {
      Serial.print("HTTP GET request failed 1, error: ");
      Serial.println(http1.errorToString(httpResponseCode1).c_str());
    }
    http1.end(); // 关闭第一个HTTP连接
  }
  // delay(10000);  // 延时10秒
}



// 实现 readPH 函数
double readPH(ph_meter_setting_t ph_meter) {
  int sensorValue = analogRead(ph_meter.pin);  // 读取 pH meter 的传感器值
  double voltage = sensorValue * (3.3 / 4095);  // 将传感器值转换为电压值
  return ph_meter.a * voltage + ph_meter.b;
}

// 连接WiFi网络
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");

  WiFi.begin(ssid, password); // 开始连接到WiFi网络

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // 输出分配给设备的IP地址
}