/**
  ******************************************************************************
  * @file    main.c
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   农业大棚环境监测系统主程序
  ******************************************************************************
  */

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f10x.h"     // 标准外设库
#include "Delay.h"         // 延时函数
#include "OLED.h"          // OLED显示驱动
#include "light.h"         // 光照传感器驱动
#include "DHT11.h"         // 温湿度传感器驱动
#include "ESP8266.h"       // WiFi通信模块
#include "Buzzer.h"        // 蜂鸣器报警模块
#include "stdio.h"         // 标准输入输出

/* 常量定义 ------------------------------------------------------------------*/
#define MAIN_LOOP_DELAY_MS    1000     // 主循环间隔（毫秒）
#define OLED_LINE_WIDTH        16      // OLED每行显示字符数
#define MAX_ERROR_COUNT         3      // 最大错误次数
#define NETWORK_RETRY_INTERVAL 60000   // 网络重试间隔(ms)

/* API配置 -------------------------------------------------------------------*/
char *POST = "/api/data";               // POST请求路径
char *HOST = "117.72.118.76:3000";      // 服务器地址

/* 全局变量 -----------------------------------------------------------------*/
static uint8_t dht_error_count = 0;                // 传感器错误计数
static uint8_t network_error_count = 0;            // 网络错误计数
static uint32_t last_successful_time = 0;          // 上次成功上传时间

/* 函数原型声明 --------------------------------------------------------------*/
static void System_Init(void);
static void Process_Sensor_Data(uint8_t dhtDataBuffer[], char *tempStr, char *humiStr, char *lightStr);
static void Handle_Sensor_Error(void);
static void Upload_Data(float temperature, float humidity, uint16_t light, char *statusStr);

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void)
{
    /* 局部变量定义 */
    uint8_t dhtDataBuffer[5] = {0};             // DHT11数据缓冲区
    DHT_FilteredData_t filtered_data;           // 滤波后的温湿度数据
    char tempDisplayStr[OLED_LINE_WIDTH + 1];   // 温度显示缓冲区
    char humiDisplayStr[OLED_LINE_WIDTH + 1];   // 湿度显示缓冲区
    char lightDisplayStr[OLED_LINE_WIDTH + 1];  // 光照显示缓冲区
    char sendDisplayStr[OLED_LINE_WIDTH + 1];   // 通信显示缓冲区

    /* 系统初始化 */
    System_Init();

    /* 主循环 */
    while (1)
    {
        /* 温湿度数据采集 */
        if (DHT_Get_Temp_Humi_Data(dhtDataBuffer))
        {
            /* 处理传感器数据并更新显示 */
            DHT_Get_Filtered_Data(dhtDataBuffer, &filtered_data);
            Process_Sensor_Data(dhtDataBuffer, tempDisplayStr, humiDisplayStr, lightDisplayStr);
            
            /* 数据上传处理 */
            Upload_Data(filtered_data.temperature, filtered_data.humidity, 
                       Light_Get(), sendDisplayStr);
                       
            // 重置DHT错误计数
            dht_error_count = 0;
        }
        else
        {
            /* 处理传感器读取错误 */
            Handle_Sensor_Error();
        }

        /* 系统延时 */
        Delay_ms(MAIN_LOOP_DELAY_MS);
    }
}

/**
  * @brief  处理传感器数据并显示
  * @param  dhtDataBuffer: DHT11原始数据缓冲区
  * @param  tempStr: 温度显示字符串缓冲区
  * @param  humiStr: 湿度显示字符串缓冲区
  * @param  lightStr: 光照显示字符串缓冲区
  * @retval 无
  */
static void Process_Sensor_Data(uint8_t dhtDataBuffer[], char *tempStr, char *humiStr, char *lightStr)
{
    DHT_FilteredData_t filtered_data;
    
    // 应用卡尔曼滤波处理数据
    DHT_Get_Filtered_Data(dhtDataBuffer, &filtered_data);
    
    // 获取滤波后的光照值
    uint16_t light = Light_Get();  // Light_Get内部已实现滤波
    
    // 格式化显示字符串
    sprintf(tempStr, "T:%.1lfC", filtered_data.temperature);
    sprintf(humiStr, "H:%.1lf%%", filtered_data.humidity);
    sprintf(lightStr, "Lux:%4d", light);
    
    // 更新OLED显示
    OLED_ShowString(1, 1, lightStr);
    OLED_ShowString(2, 1, tempStr);
    OLED_ShowString(3, 1, humiStr);
    
    // 根据滤波后的传感器数据控制蜂鸣器报警
    Buzzer_Control(light, (int)filtered_data.temperature, (int)filtered_data.humidity);
}

/**
  * @brief  处理传感器读取错误
  * @param  无
  * @retval 无
  */
static void Handle_Sensor_Error(void)
{
    dht_error_count++;
    
    // 显示错误信息
    char errMsg[OLED_LINE_WIDTH + 1];
    sprintf(errMsg, "Sen err %d/%d    ", dht_error_count, MAX_ERROR_COUNT);
    OLED_ShowString(2, 1, errMsg);
    OLED_ShowString(3, 1, "                ");
    
    // 连续失败次数达到阈值，尝试重新初始化
    if (dht_error_count >= MAX_ERROR_COUNT)
    {
        OLED_ShowString(2, 1, "Reinit sensor   ");
        // 重新初始化传感器代码
        Delay_ms(1000);
        dht_error_count = 0;
    }
}

/**
  * @brief  上传数据到服务器
  * @param  temperature: 温度数据
  * @param  humidity: 湿度数据
  * @param  light: 光照数据
  * @param  statusStr: 状态显示字符串缓冲区
  * @retval 无
  */
static void Upload_Data(float temperature, float humidity, uint16_t light, char *statusStr)
{
    uint32_t current_time = 0; // 此处应使用系统滴答计时器
    uint32_t code;
    
    // 检查是否需要上传数据（初次或上次失败后到达重试时间）
    if (network_error_count == 0 || 
        (current_time - last_successful_time > NETWORK_RETRY_INTERVAL))
    {
        // 拼接JSON格式的传感器数据
        char json[250];
        sprintf(json, "{\"temperature\": %.1f, \"humidity\": %.1f, \"light\": %d}",
                temperature, humidity, light);

        // 发送HTTP POST请求到服务器
        if (ESP8266_Send_http_post(POST, HOST, json))
        {
            // 处理HTTP响应
            if (ESP8266_Receive_http_response(&code))
            {
                // 成功接收到响应，更新状态
                sprintf(statusStr, "send:%4d       ", code);
                last_successful_time = current_time;
                network_error_count = 0;
            }
            else
            {
                // 响应接收失败，增加错误计数
                network_error_count++;
                sprintf(statusStr, "err %d/%d       ", network_error_count, MAX_ERROR_COUNT);
                
                // 错误次数达到阈值，重启WiFi模块
                if (network_error_count >= MAX_ERROR_COUNT)
                {
                    sprintf(statusStr, "restart wifi...");
                    OLED_ShowString(4, 1, statusStr);
                    ESP8266_Restart();
                    network_error_count = 0;
                    Delay_ms(3000); // 等待重启完成
                    return;
                }
            }
        }
        else
        {
            // 发送请求失败，增加错误计数
            network_error_count++;
            sprintf(statusStr, "send err %d/%d  ", network_error_count, MAX_ERROR_COUNT);
        }
    }
    else
    {
        // 等待网络重试间隔
        sprintf(statusStr, "wait to retry  ");
    }
    
    // 更新网络状态显示
    OLED_ShowString(4, 1, statusStr);
}

/**
  * @brief  系统初始化
  * @param  无
  * @retval 无
  * @note   初始化所有外设模块
  */
static void System_Init(void)
{
    /* 初始化OLED显示 */
    OLED_Init();
    OLED_ShowString(1, 1, "System Init...");

    /* 初始化光照传感器 */
    Light_Init();
    
    /* 初始化DHT11卡尔曼滤波器 */
    DHT_Filter_Init();

    /* 初始化ESP8266 */
    ESP8266_Init();

    /* 初始化蜂鸣器 */
    Buzzer_Init();

    /* 延时确保传感器稳定 */
    Delay_ms(100);

    /* 清屏显示 */
    OLED_Clear();
}

/******************* (C) COPYRIGHT 2024 农业大棚监控小组 *****END OF FILE******/
