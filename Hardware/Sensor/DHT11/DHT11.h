/**
  ******************************************************************************
  * @file    DHT11.h
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   DHT11温湿度传感器驱动头文件
  ******************************************************************************
  */

#ifndef __DHT11_H
#define __DHT11_H

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f10x.h"
#include "Delay.h"
#include <stdlib.h>
#include <stdio.h>
#include "kalman.h"

/* 硬件接口定义 --------------------------------------------------------------*/
#define DHT_GPIO_PORT    GPIOB                   // DHT11连接的GPIO端口
#define DHT_GPIO_PIN     GPIO_Pin_5              // DHT11连接的GPIO引脚
#define DHT_RCC_PORT     RCC_APB2Periph_GPIOB    // DHT11 GPIO时钟

/* 错误码定义 ----------------------------------------------------------------*/
#define DHT_OK           1   // 读取成功
#define DHT_ERROR        0   // 读取失败
#define DHT_TIMEOUT      2   // 通信超时

/* 数据结构定义 --------------------------------------------------------------*/
/**
  * @brief  DHT11滤波后的温湿度数据结构体
  */
typedef struct {
    double temperature;   // 滤波后的温度值
    double humidity;      // 滤波后的湿度值
} DHT_FilteredData_t;

/* 函数声明 ------------------------------------------------------------------*/
/**
  * @brief  DHT11 GPIO初始化
  * @param  Mode: 指定输入或输出模式
  * @retval 无
  */
void DHT_GPIO_Init(GPIOMode_TypeDef Mode);

/**
  * @brief  DHT11起始信号函数
  * @param  无
  * @retval 1:成功 0:失败
  */
uint8_t DHT_Start(void);

/**
  * @brief  获取DHT11的一个字节数据
  * @param  无
  * @retval 接收到的8位数据
  */
uint8_t DHT_Get_Byte_Data(void);

/**
  * @brief  获取DHT11的温湿度数据
  * @param  buffer[]: 存储数据的数组，至少5个元素
  *                   buffer[0]: 湿度整数部分
  *                   buffer[1]: 湿度小数部分
  *                   buffer[2]: 温度整数部分
  *                   buffer[3]: 温度小数部分
  *                   buffer[4]: 校验和
  * @retval 1:成功 0:失败
  */
uint8_t DHT_Get_Temp_Humi_Data(uint8_t buffer[]);

/**
  * @brief  初始化DHT11卡尔曼滤波器
  * @param  无
  * @retval 无
  */
void DHT_Filter_Init(void);

/**
  * @brief  获取滤波后的温湿度数据
  * @param  buffer[]: 原始数据缓冲区，与DHT_Get_Temp_Humi_Data相同格式
  * @param  filtered_data: 指向存储滤波后数据的结构体
  * @retval 1:成功 0:失败
  */
uint8_t DHT_Get_Filtered_Data(uint8_t buffer[], DHT_FilteredData_t *filtered_data);

/**
  * @brief  获取处理后的温湿度数据(简化接口)
  * @param  filtered_data: 指向存储滤波后数据的结构体
  * @retval 1:成功 0:失败
  * @note   此函数封装了数据采集和滤波处理，应用层只需调用此函数即可
  */
uint8_t DHT_GetProcessedData(DHT_FilteredData_t *filtered_data);

#endif /* __DHT11_H */

/* 文件结束 -----------------------------------------------------------------*/
