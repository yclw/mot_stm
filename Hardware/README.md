# 硬件模块说明文档

## 目录
- [1. 概述](#1-概述)
- [2. 传感器模块](#2-传感器模块)
  - [2.1 DHT11温湿度传感器](#21-dht11温湿度传感器)
  - [2.2 光照传感器](#22-光照传感器)
- [3. 显示模块](#3-显示模块)
  - [3.1 OLED显示屏](#31-oled显示屏)
- [4. 通信模块](#4-通信模块)
  - [4.1 ESP8266 WiFi模块](#41-esp8266-wifi模块)
- [5. 执行器模块](#5-执行器模块)
  - [5.1 蜂鸣器模块](#51-蜂鸣器模块)
- [6. 中间件](#6-中间件)
  - [6.1 卡尔曼滤波器](#61-卡尔曼滤波器)

## 1. 概述

本文档详细描述了农业大棚环境监测系统中使用的各个硬件模块，包括接口定义、功能说明以及驱动使用方法，为系统开发和维护提供参考。

## 2. 传感器模块

### 2.1 DHT11温湿度传感器

DHT11是一款数字温湿度传感器，用于测量环境温度和湿度。

#### 2.1.1 硬件连接

| 引脚 | 连接到 | 说明 |
|------|--------|------|
| DATA | PB5    | 数据线 |
| VCC  | 3.3V   | 电源 |
| GND  | GND    | 接地 |

#### 2.1.2 功能特点

- 温度测量范围：0-50℃，精度±2℃
- 湿度测量范围：20-90%RH，精度±5%RH
- 单总线数据格式
- 数字信号输出

#### 2.1.3 驱动API

```c
/* 初始化DHT11引脚 */
void DHT_GPIO_Init(GPIOMode_TypeDef Mode);

/* DHT11启动信号 */
uint8_t DHT_Start(void);

/* 获取一个字节数据 */
uint8_t DHT_Get_Byte_Data(void);

/* 获取温湿度数据 */
uint8_t DHT_Get_Temp_Humi_Data(uint8_t buffer[]);

/* 卡尔曼滤波初始化 */
void DHT_Filter_Init(void);

/* 获取滤波后的数据 */
uint8_t DHT_Get_Filtered_Data(uint8_t buffer[], DHT_FilteredData_t *filtered_data);

/* 获取处理后的温湿度数据(简化接口) */
uint8_t DHT_GetProcessedData(DHT_FilteredData_t *filtered_data);
```

#### 2.1.4 使用示例

```c
DHT_FilteredData_t filtered_data;

/* 初始化滤波器 */
DHT_Filter_Init();

/* 方法一: 低级接口（分步处理） */
uint8_t dhtDataBuffer[5];
if (DHT_Get_Temp_Humi_Data(dhtDataBuffer)) {
    DHT_Get_Filtered_Data(dhtDataBuffer, &filtered_data);
    printf("温度: %.1lf°C, 湿度: %.1lf%%\r\n", 
           filtered_data.temperature, filtered_data.humidity);
}

/* 方法二: 简化接口（推荐使用） */
if (DHT_GetProcessedData(&filtered_data)) {
    printf("温度: %.1lf°C, 湿度: %.1lf%%\r\n", 
           filtered_data.temperature, filtered_data.humidity);
}
```

### 2.2 光照传感器

光照传感器用于测量环境光照强度，采用ADC采集模拟信号。

#### 2.2.1 硬件连接

| 引脚 | 连接到 | 说明 |
|------|--------|------|
| OUT  | PA1    | 模拟信号输出 |
| VCC  | 3.3V   | 电源 |
| GND  | GND    | 接地 |

#### 2.2.2 功能特点

- 测量范围：0-1000 Lux
- 模拟信号输出
- 响应迅速

#### 2.2.3 驱动API

```c
/* 初始化光照传感器，配置ADC */
void Light_Init(void);

/* 获取光照值 */
uint16_t Light_Get(void);
```

#### 2.2.4 使用示例

```c
/* 初始化 */
Light_Init();

/* 获取光照值 */
uint16_t light = Light_Get();
printf("光照强度: %d Lux\r\n", light);
```

## 3. 显示模块

### 3.1 OLED显示屏

OLED显示屏用于直观展示传感器数据和系统状态。

#### 3.1.1 硬件连接

| 引脚 | 连接到 | 说明 |
|------|--------|------|
| SCL  | PB6    | 时钟线 |
| SDA  | PB7    | 数据线 |
| VCC  | 3.3V   | 电源 |
| GND  | GND    | 接地 |

#### 3.1.2 功能特点

- 屏幕尺寸：0.96寸
- 分辨率：128x64像素
- I2C接口
- 支持显示字符、数字和图形

#### 3.1.3 驱动API

```c
/* 初始化OLED */
void OLED_Init(void);

/* 清屏 */
void OLED_Clear(void);

/* 显示字符 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);

/* 显示字符串 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);

/* 显示整数 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/* 显示有符号整数 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);

/* 显示16进制数 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/* 显示2进制数 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
```

#### 3.1.4 使用示例

```c
/* 初始化 */
OLED_Init();
OLED_Clear();

/* 显示内容 */
OLED_ShowString(1, 1, "Temp: 25.5C");
OLED_ShowString(2, 1, "Humi: 65.0%");
OLED_ShowString(3, 1, "Light: 500 Lux");
```

## 4. 通信模块

### 4.1 ESP8266 WiFi模块

ESP8266用于将采集的传感器数据上传到云服务器。

#### 4.1.1 硬件连接

| 引脚 | 连接到 | 说明 |
|------|--------|------|
| TX   | PA3    | USART2_RX |
| RX   | PA2    | USART2_TX |
| VCC  | 3.3V   | 电源 |
| GND  | GND    | 接地 |
| RST  | PB8    | 复位引脚 |
| EN   | 3.3V   | 使能引脚 |

#### 4.1.2 功能特点

- 支持802.11 b/g/n协议
- 支持TCP/IP协议栈
- AT指令集
- HTTP通信支持

#### 4.1.3 驱动API

```c
/* 初始化ESP8266 */
void ESP8266_Init(void);

/* 重启ESP8266 */
void ESP8266_Restart(void);

/* 发送HTTP POST请求 */
int ESP8266_Send_http_post(char *POST, char *Host, char *json);

/* 接收HTTP响应 */
int ESP8266_Receive_http_response(uint32_t *code);
```

#### 4.1.4 使用示例

```c
char json[100];
uint32_t code;

/* 初始化 */
ESP8266_Init();

/* 发送数据 */
sprintf(json, "{\"temperature\": %.1f, \"humidity\": %.1f, \"light\": %d}",
        25.5, 65.0, 500);
if (ESP8266_Send_http_post("/api/data", "server.com", json)) {
    if (ESP8266_Receive_http_response(&code)) {
        printf("HTTP响应码: %lu\r\n", code);
    }
}
```

## 5. 执行器模块

### 5.1 蜂鸣器模块

蜂鸣器用于在环境参数异常时进行报警提示。

#### 5.1.1 硬件连接

| 引脚 | 连接到 | 说明 |
|------|--------|------|
| SIG  | PA8    | 控制信号 |
| VCC  | 3.3V   | 电源 |
| GND  | GND    | 接地 |

#### 5.1.2 功能特点

- 有源蜂鸣器
- 多种报警模式：连续、间歇
- 可配置环境阈值

#### 5.1.3 驱动API

```c
/* 初始化蜂鸣器 */
void Buzzer_Init(void);

/* 蜂鸣器打开 */
void Buzzer_ON(void);

/* 蜂鸣器关闭 */
void Buzzer_OFF(void);

/* 蜂鸣器短鸣 */
void Buzzer_Beep(uint16_t duration_ms);

/* 设置环境阈值 */
void Buzzer_SetThreshold(EnvThreshold_t *threshold);

/* 根据传感器数据控制蜂鸣器 */
void Buzzer_Control(uint16_t Light, uint16_t temp, uint16_t hum);

/* 设置报警模式 */
void Buzzer_SetMode(BuzzerMode_t mode);
```

#### 5.1.4 使用示例

```c
/* 初始化 */
Buzzer_Init();

/* 设置阈值 */
EnvThreshold_t threshold = {
    .light_min = 100,
    .light_max = 800,
    .temp_min = 18,
    .temp_max = 30,
    .humi_min = 40,
    .humi_max = 80
};
Buzzer_SetThreshold(&threshold);

/* 设置报警模式 */
Buzzer_SetMode(BUZZER_MODE_INTERMITTENT);

/* 根据传感器数据控制 */
Buzzer_Control(500, 25, 65);
```

## 6. 中间件

### 6.1 卡尔曼滤波器

卡尔曼滤波器用于滤除传感器数据中的噪声，提高数据精度。

#### 6.1.1 功能特点

- 一维卡尔曼滤波
- 可配置噪声参数
- 实时滤波处理

#### 6.1.2 API

```c
/* 初始化卡尔曼滤波器 */
void KalmanFilter_Init(KalmanFilter_t *filter, double init_value, double Q, double R);

/* 更新滤波器并返回滤波后的值 */
double KalmanFilter_Update(KalmanFilter_t *filter, double measurement);
```

#### 6.1.3 使用示例

```c
KalmanFilter_t filter;

/* 初始化 */
KalmanFilter_Init(&filter, 25.0, 0.01, 0.1);

/* 实时滤波 */
double raw_temp = 25.5;
double filtered_temp = KalmanFilter_Update(&filter, raw_temp);
printf("原始温度: %.1lf, 滤波后: %.1lf\r\n", raw_temp, filtered_temp);
``` 