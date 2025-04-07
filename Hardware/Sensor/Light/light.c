/**
  ******************************************************************************
  * @file    light.c
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   光照传感器驱动实现
  ******************************************************************************
  */

/* 包含头文件 ----------------------------------------------------------------*/
#include "light.h"

/* 私有变量 ------------------------------------------------------------------*/
static KalmanFilter_t light_filter; // 光照传感器卡尔曼滤波器实例

/* 私有函数声明 --------------------------------------------------------------*/
static void AD_Init(void);
static uint16_t AD_GetValue(uint8_t ADC_Channel);

/* ADC模块 -------------------------------------------------------------------*/
/**
  * @brief  ADC模块初始化
  * @param  无
  * @retval 无
  * @note   配置ADC1和GPIOA的时钟，初始化ADC参数并执行校准
  */
static void AD_Init(void)
{
    /* 时钟配置 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // 设置ADC时钟为PCLK2的6分频

    /* GPIO配置 */
    GPIO_InitTypeDef GPIO_InitStruct = {
        .GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2,  // PA1和PA2作为ADC输入
        .GPIO_Mode  = GPIO_Mode_AIN,            // 模拟输入模式
        .GPIO_Speed = GPIO_Speed_50MHz
    };
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC参数初始化 */
    ADC_InitTypeDef ADC_InitStruct = {
        .ADC_Mode               = ADC_Mode_Independent,     // 独立模式
        .ADC_ScanConvMode       = DISABLE,                   // 非扫描模式
        .ADC_ContinuousConvMode = DISABLE,                   // 单次转换模式
        .ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None, // 软件触发
        .ADC_DataAlign          = ADC_DataAlign_Right,       // 数据右对齐
        .ADC_NbrOfChannel       = 1                          // 转换通道数
    };
    ADC_Init(ADC1, &ADC_InitStruct);

    /* 使能ADC并校准 */
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));  // 等待复位校准完成
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));       // 等待校准完成
}

/**
  * @brief  获取指定通道的ADC值
  * @param  ADC_Channel: ADC输入通道号
  * @retval 12位ADC转换结果
  */
static uint16_t AD_GetValue(uint8_t ADC_Channel)
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);      // 启动转换
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)); // 等待转换完成
    return ADC_GetConversionValue(ADC1);         // 自动清除EOC标志
}

/* 光照传感器模块 -------------------------------------------------------------*/
/**
  * @brief  光照传感器初始化
  * @param  无
  * @retval 无
  * @note   执行ADC初始化并进行20次空读取稳定ADC
  */
void Light_Init(void)
{
    AD_Init();
    
    /* 进行20次空读取以稳定读数 */
    uint8_t i;
    for(i = 0; i < 20; i++) {
        AD_GetValue(ADC_Channel_1);  // 通道1对应光照传感器
    }
    
    /* 获取初始光照值用于初始化卡尔曼滤波器 */
    uint16_t init_adc_value = AD_GetValue(ADC_Channel_1);
    float init_light = 1000.0f - (init_adc_value / 4095.0f) * 1000.0f;
    
    /* 初始化卡尔曼滤波器 
     * Q = 0.01: 较小的过程噪声，因为光照变化通常较为缓慢
     * R = 10.0: 较大的测量噪声，考虑到ADC读数可能有波动
     */
    KalmanFilter_Init(&light_filter, (double)init_light, 0.01, 10.0);
}

/**
  * @brief  获取当前光照强度
  * @param  无
  * @retval 0-1000范围的光照强度值（0最暗，1000最亮）
  * @note   转换公式假设ADC满量程对应反向比例关系
  */
uint16_t Light_Get(void)
{
    uint16_t adc_value = AD_GetValue(ADC_Channel_1);
    
    /* 将ADC值转换为光照强度：
       - ADC满量程4095对应0光照
       - ADC最小值0对应1000光照 */
    float light_raw = 1000.0f - (adc_value / 4095.0f) * 1000.0f;
    
    /* 使用卡尔曼滤波器处理光照数据 */
    double light_filtered = KalmanFilter_Update(&light_filter, (double)light_raw);
    
    /* 四舍五入处理并限制范围 */
    return (uint16_t)(light_filtered + 0.5);
}

/* 文件结束 -----------------------------------------------------------------*/
