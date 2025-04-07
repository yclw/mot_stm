/**
  ******************************************************************************
  * @file    Buzzer.h
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-11
  * @brief   蜂鸣器报警驱动头文件
  ******************************************************************************
  */

#ifndef __BUZZER_H
#define __BUZZER_H

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stddef.h>    // 定义NULL

/* 类型定义 ------------------------------------------------------------------*/
/**
  * @brief  环境参数阈值结构体
  */
typedef struct {
    uint16_t light_min;    // 光照最小阈值
    uint16_t light_max;    // 光照最大阈值
    uint16_t temp_min;     // 温度最小阈值
    uint16_t temp_max;     // 温度最大阈值
    uint16_t humi_min;     // 湿度最小阈值
    uint16_t humi_max;     // 湿度最大阈值
} EnvThreshold_t;

/**
  * @brief  蜂鸣器报警模式枚举
  */
typedef enum {
    BUZZER_MODE_OFF,       // 关闭报警
    BUZZER_MODE_CONTINUOUS,// 连续报警
    BUZZER_MODE_INTERMITTENT// 间歇性报警
} BuzzerMode_t;

/* 函数声明 ------------------------------------------------------------------*/
/**
  * @brief  蜂鸣器初始化
  * @param  无
  * @retval 无
  */
void Buzzer_Init(void);

/**
  * @brief  蜂鸣器打开
  * @param  无
  * @retval 无
  */
void Buzzer_ON(void);

/**
  * @brief  蜂鸣器关闭
  * @param  无
  * @retval 无
  */
void Buzzer_OFF(void);

/**
  * @brief  蜂鸣器短鸣一段时间
  * @param  duration_ms: 蜂鸣持续时间(ms)
  * @retval 无
  */
void Buzzer_Beep(uint16_t duration_ms);

/**
  * @brief  设置环境参数阈值
  * @param  threshold: 阈值结构体指针
  * @retval 无
  */
void Buzzer_SetThreshold(EnvThreshold_t *threshold);

/**
  * @brief  根据传感器数据控制蜂鸣器
  * @param  Light: 光照强度值
  * @param  temp: 温度值
  * @param  hum: 湿度值
  * @retval 无
  */
void Buzzer_Control(uint16_t Light, uint16_t temp, uint16_t hum);

/**
  * @brief  设置报警模式
  * @param  mode: 蜂鸣器报警模式
  * @retval 无
  */
void Buzzer_SetMode(BuzzerMode_t mode);

#endif /* __BUZZER_H */

/* 文件结束 -----------------------------------------------------------------*/

