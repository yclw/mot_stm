/**
  ******************************************************************************
  * @file    ESP8266.h
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   ESP8266 WiFi模块驱动头文件
  ******************************************************************************
  */

#ifndef __ESP8266_H
#define __ESP8266_H

/* 包含头文件 ----------------------------------------------------------------*/
#include <stdint.h>
//#include "Serial.h"

/* 函数声明 ------------------------------------------------------------------*/
/**
  * @brief  初始化ESP8266
  * @param  无
  * @retval 无
  */
void ESP8266_Init(void);

/**
  * @brief  重启ESP8266
  * @param  无
  * @retval 无
  */
void ESP8266_Restart(void);

/**
  * @brief  发送HTTP POST请求
  * @param  POST: POST请求路径
  * @param  Host: 主机地址和端口
  * @param  json: JSON格式的数据
  * @retval 1:发送成功 0:发送失败
  */
int ESP8266_Send_http_post(char *POST, char *Host, char *json);

/**
  * @brief  接收HTTP响应并解析状态码
  * @param  code: 解析出的HTTP状态码存放地址
  * @retval 1:接收并解析成功 0:接收失败或解析失败
  */
int ESP8266_Receive_http_response(uint32_t *code);

#endif /* __ESP8266_H */

/* 文件结束 -----------------------------------------------------------------*/
