/**
  ******************************************************************************
  * @file    app.h
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   应用层头文件
  * @note    本文件包含应用层函数声明和全局变量定义
  *          定义了系统初始化、主循环、传感器数据处理等函数接口
  ******************************************************************************
  */

#ifndef __APP_H
#define __APP_H

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f10x.h"
#include "../Config/config.h"

/* 函数声明 ----------------------------------------------------------------*/
/**
  * @brief  系统初始化
  * @param  无
  * @retval 无
  * @note   初始化所有外设模块，包括OLED、传感器、WiFi等
  */
void App_Init(void);

/**
  * @brief  主循环处理函数
  * @param  无
  * @retval 无
  * @note   循环处理传感器数据采集、显示和上传
  */
void App_MainLoop(void);

/**
  * @brief  处理传感器数据
  * @param  无
  * @retval 无
  * @note   处理温湿度数据，更新显示并控制报警
  */
void App_ProcessSensorData(void);

/**
  * @brief  处理传感器错误
  * @param  无
  * @retval 无
  * @note   处理传感器读取失败情况，显示错误信息
  */
void App_HandleSensorError(void);

/**
  * @brief  上传数据到服务器
  * @param  temperature: 温度数据
  * @param  humidity: 湿度数据
  * @param  light: 光照数据
  * @retval 无
  * @note   将传感器数据通过WiFi上传到服务器
  */
void App_UploadData(float temperature, float humidity, uint16_t light);

#endif /* __APP_H */ 

/* 文件结束 -----------------------------------------------------------------*/
