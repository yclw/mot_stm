/**
  ******************************************************************************
  * @file    Buzzer.c
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-11
  * @brief   蜂鸣器报警驱动程序
  ******************************************************************************
  */

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f10x.h"    // 设备头文件
#include "Delay.h"        // 延时函数
#include "Buzzer.h"       // 蜂鸣器控制接口
#include <stddef.h>       // 定义NULL

/* 全局变量 ------------------------------------------------------------------*/
// 默认环境阈值
static EnvThreshold_t env_threshold = {
	.light_min = 200,    // 光照最小阈值
	.light_max = 700,    // 光照最大阈值
	.temp_min = 10,      // 温度最小阈值
	.temp_max = 30,      // 温度最大阈值
	.humi_min = 30,      // 湿度最小阈值
	.humi_max = 70       // 湿度最大阈值
};

// 当前蜂鸣器模式
static BuzzerMode_t buzzer_mode = BUZZER_MODE_CONTINUOUS;

// 蜂鸣器状态（用于间歇模式）
static uint8_t buzzer_state = 0;
static uint32_t last_toggle_time = 0;

/**
  * @brief  蜂鸣器初始化
  * @param  无
  * @retval 无
  */
void Buzzer_Init(void)
{
	// 使能蜂鸣器GPIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	// 配置蜂鸣器GPIO为推挽输出
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// 默认蜂鸣器关闭
	GPIO_SetBits(GPIOB, GPIO_Pin_13);
}

/**
  * @brief  蜂鸣器打开
  * @param  无
  * @retval 无
  */
void Buzzer_ON(void)
{	
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);
}

/**
  * @brief  蜂鸣器关闭
  * @param  无
  * @retval 无
  */
void Buzzer_OFF(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_13);
}

/**
  * @brief  蜂鸣器短鸣一段时间
  * @param  duration_ms: 蜂鸣持续时间(ms)
  * @retval 无
  */
void Buzzer_Beep(uint16_t duration_ms)
{
	Buzzer_ON();
	Delay_ms(duration_ms);
	Buzzer_OFF();
}

/**
  * @brief  设置环境参数阈值
  * @param  threshold: 阈值结构体指针
  * @retval 无
  */
void Buzzer_SetThreshold(EnvThreshold_t *threshold)
{
	if (threshold != NULL) {
		env_threshold = *threshold;
	}
}

/**
  * @brief  设置报警模式
  * @param  mode: 报警模式
  * @retval 无
  */
void Buzzer_SetMode(BuzzerMode_t mode)
{
	buzzer_mode = mode;
	
	// 切换模式时关闭蜂鸣器
	if (mode == BUZZER_MODE_OFF) {
		Buzzer_OFF();
	}
}

/**
  * @brief  更新间歇式报警状态
  * @param  无
  * @retval 无
  * @note   此函数需要在主循环中定期调用
  */
static void Buzzer_UpdateIntermittent(void)
{
	// 使用系统计数替代实际系统时钟
	static uint32_t beep_interval = 500; // 500ms间隔
	uint32_t current_time = 0;
	
	// 简化的时间计算，实际应使用系统滴答计时器
	current_time++;
	
	if (current_time - last_toggle_time >= beep_interval) {
		buzzer_state = !buzzer_state;
		last_toggle_time = current_time;
		
		// 根据状态控制蜂鸣器
		if (buzzer_state) {
			Buzzer_ON();
		} else {
			Buzzer_OFF();
		}
	}
}

/**
  * @brief  根据传感器数据控制蜂鸣器
  * @param  Light: 光照强度
  * @param  temp: 温度值
  * @param  hum: 湿度值
  * @retval 无
  */
void Buzzer_Control(uint16_t Light, uint16_t temp, uint16_t hum)
{
	// 检查是否超出阈值范围
	uint8_t alarm_condition = (Light < env_threshold.light_min || 
							   Light > env_threshold.light_max || 
							   temp < env_threshold.temp_min || 
							   temp > env_threshold.temp_max || 
							   hum < env_threshold.humi_min || 
							   hum > env_threshold.humi_max);
	
	// 根据报警模式和条件控制蜂鸣器
	if (alarm_condition) {
		switch (buzzer_mode) {
			case BUZZER_MODE_CONTINUOUS:
				Buzzer_ON();
				break;
			
			case BUZZER_MODE_INTERMITTENT:
				Buzzer_UpdateIntermittent();
				break;
			
			case BUZZER_MODE_OFF:
			default:
				Buzzer_OFF();
				break;
		}
	} else {
		Buzzer_OFF();
	}
}

/* 文件结束 -----------------------------------------------------------------*/


