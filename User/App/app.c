/**
  ******************************************************************************
  * @file    app.c
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   应用层实现文件
  * @note    本文件实现应用层功能，包括系统初始化、主循环、
  *          传感器数据处理和网络通信等功能
  ******************************************************************************
  */

/* 包含头文件 ----------------------------------------------------------------*/
#include "app.h"
#include "oled.h"
#include "dht11.h"
#include "light.h"
#include "esp8266.h"
#include "buzzer.h"
#include "../Config/config.h"
#include <stdio.h>

/* 全局变量 ----------------------------------------------------------------*/
static uint8_t dht_error_count = 0;                // 传感器错误计数
static uint8_t network_error_count = 0;            // 网络错误计数
static uint32_t last_successful_time = 0;          // 上次成功上传时间

/**
  * @brief  系统初始化
  * @param  无
  * @retval 无
  * @note   初始化所有外设模块，包括OLED、传感器、WiFi等
  */
void App_Init(void)
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

/**
  * @brief  主循环处理函数
  * @param  无
  * @retval 无
  * @note   循环处理传感器数据采集、显示和上传
  */
void App_MainLoop(void)
{
    /* 处理传感器数据 - 数据采集和处理已封装在传感器驱动层 */
    App_ProcessSensorData();

    /* 系统延时 */
    Delay_ms(MAIN_LOOP_DELAY_MS);
}

/**
  * @brief  处理传感器数据
  * @param  无
  * @retval 无
  * @note   处理温湿度数据，更新显示并控制报警
  */
void App_ProcessSensorData(void)
{
    DHT_FilteredData_t filtered_data;
    char tempDisplayStr[OLED_LINE_WIDTH + 1];
    char humiDisplayStr[OLED_LINE_WIDTH + 1];
    char lightDisplayStr[OLED_LINE_WIDTH + 1];
    
    /* 获取处理后的温湿度数据 */
    if (!DHT_GetProcessedData(&filtered_data)) {
        App_HandleSensorError();
        return;
    }
    
    /* 重置错误计数 */
    dht_error_count = 0;
    
    /* 获取光照值 */
    uint16_t light = Light_Get();
    
    /* 格式化显示字符串 */
    sprintf(tempDisplayStr, "T:%.1lfC", filtered_data.temperature);
    sprintf(humiDisplayStr, "H:%.1lf%%", filtered_data.humidity);
    sprintf(lightDisplayStr, "Lux:%4d", light);
    
    /* 更新OLED显示 */
    OLED_ShowString(1, 1, lightDisplayStr);
    OLED_ShowString(2, 1, tempDisplayStr);
    OLED_ShowString(3, 1, humiDisplayStr);
    
    /* 根据滤波后的传感器数据控制蜂鸣器报警 */
    Buzzer_Control(light, (int)filtered_data.temperature, (int)filtered_data.humidity);
    
    /* 上传数据 */
    App_UploadData(filtered_data.temperature, filtered_data.humidity, light);
}

/**
  * @brief  处理传感器错误
  * @param  无
  * @retval 无
  * @note   处理传感器读取失败情况，显示错误信息
  */
void App_HandleSensorError(void)
{
    dht_error_count++;
    
    /* 显示错误信息 */
    char errMsg[OLED_LINE_WIDTH + 1];
    sprintf(errMsg, "Sen err %d/%d    ", dht_error_count, MAX_ERROR_COUNT);
    OLED_ShowString(2, 1, errMsg);
    OLED_ShowString(3, 1, "                ");
    
    /* 连续失败次数达到阈值，尝试重新初始化 */
    if (dht_error_count >= MAX_ERROR_COUNT)
    {
        OLED_ShowString(2, 1, "Reinit sensor   ");
        /* 重新初始化传感器代码 */
        Delay_ms(1000);
        dht_error_count = 0;
    }
}

/**
  * @brief  上传数据到服务器
  * @param  temperature: 温度数据
  * @param  humidity: 湿度数据
  * @param  light: 光照数据
  * @retval 无
  * @note   将传感器数据通过WiFi上传到服务器
  */
void App_UploadData(float temperature, float humidity, uint16_t light)
{
    uint32_t current_time = 0; /* 此处应使用系统滴答计时器 */
    uint32_t code;
    char statusStr[OLED_LINE_WIDTH + 1];
    
    /* 检查是否需要上传数据 */
    if (network_error_count == 0 || 
        (current_time - last_successful_time > NETWORK_RETRY_INTERVAL))
    {
        /* 拼接JSON格式的传感器数据 */
        char json[250];
        sprintf(json, "{\"temperature\": %.1f, \"humidity\": %.1f, \"light\": %d}",
                temperature, humidity, light);

        /* 发送HTTP POST请求到服务器 */
        if (ESP8266_Send_http_post(POST_PATH, SERVER_HOST, json))
        {
            /* 处理HTTP响应 */
            if (ESP8266_Receive_http_response(&code))
            {
                sprintf(statusStr, "send:%4d       ", code);
                last_successful_time = current_time;
                network_error_count = 0;
            }
            else
            {
                network_error_count++;
                sprintf(statusStr, "err %d/%d       ", network_error_count, MAX_ERROR_COUNT);
                
                if (network_error_count >= MAX_ERROR_COUNT)
                {
                    sprintf(statusStr, "restart wifi...");
                    OLED_ShowString(4, 1, statusStr);
                    ESP8266_Restart();
                    network_error_count = 0;
                    Delay_ms(3000);
                    return;
                }
            }
        }
        else
        {
            network_error_count++;
            sprintf(statusStr, "send err %d/%d  ", network_error_count, MAX_ERROR_COUNT);
        }
    }
    else
    {
        sprintf(statusStr, "wait to retry  ");
    }
    
    OLED_ShowString(4, 1, statusStr);
}

/* 文件结束 -----------------------------------------------------------------*/
