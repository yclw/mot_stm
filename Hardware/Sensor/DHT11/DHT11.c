/**
  ******************************************************************************
  * @file    DHT11.c
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   DHT11温湿度传感器驱动实现
  ******************************************************************************
  */

/* 包含头文件 ----------------------------------------------------------------*/
#include "DHT11.h"
#include "Delay.h"

/* 私有宏定义 ----------------------------------------------------------------*/
#define DHT_TIMEOUT_VALUE  1000   // 通信超时时间（单位：循环次数）

/* 私有变量 -----------------------------------------------------------------*/
static KalmanFilter_t temp_filter;  // 温度卡尔曼滤波器
static KalmanFilter_t humi_filter;  // 湿度卡尔曼滤波器
static uint8_t is_filter_initialized = 0;  // 滤波器初始化标志

/**
  * @brief  DHT11 GPIO初始化函数
  * @param  Mode: 指定输入或输出模式
  * @retval 无
  */
void DHT_GPIO_Init(GPIOMode_TypeDef Mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 使能GPIO时钟
	RCC_APB2PeriphClockCmd(DHT_RCC_PORT, ENABLE);
	
	// 配置GPIO
	if (Mode == GPIO_Mode_Out_PP) {
		GPIO_SetBits(DHT_GPIO_PORT, DHT_GPIO_PIN);  // 初始化为高电平
	}
	
	GPIO_InitStructure.GPIO_Mode = Mode;
	GPIO_InitStructure.GPIO_Pin = DHT_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(DHT_GPIO_PORT, &GPIO_InitStructure);
}

/**
  * @brief  DHT11模块起始信号函数
  * @param  无
  * @retval 1:成功 0:失败或超时
  */
uint8_t DHT_Start(void)
{
	// 配置GPIO为输出模式
	DHT_GPIO_Init(GPIO_Mode_Out_PP);
	
	// 主机发送起始信号：输出低电平至少18ms，然后输出高电平20-40us
	GPIO_ResetBits(DHT_GPIO_PORT, DHT_GPIO_PIN);    // 拉低总线
	Delay_ms(20);                                   // 延时20ms
	GPIO_SetBits(DHT_GPIO_PORT, DHT_GPIO_PIN);      // 拉高总线
	
	// 配置GPIO为输入模式，准备接收DHT11响应
	DHT_GPIO_Init(GPIO_Mode_IN_FLOATING);
	Delay_us(20);  // 延时等待DHT11响应
	
	// 超时计数器
	uint16_t timeout = 0;
	
	// 等待DHT11拉低电平响应
	while(GPIO_ReadInputDataBit(DHT_GPIO_PORT, DHT_GPIO_PIN)) {
		Delay_us(1);
		if(++timeout > DHT_TIMEOUT_VALUE) return 0; // 超时退出
	}
	
	// 等待DHT11的80us低电平响应信号结束
	timeout = 0;
	while(!GPIO_ReadInputDataBit(DHT_GPIO_PORT, DHT_GPIO_PIN)) {
		Delay_us(1);
		if(++timeout > DHT_TIMEOUT_VALUE) return 0; // 超时退出
	}
	
	// 等待DHT11的80us高电平响应信号结束
	timeout = 0;
	while(GPIO_ReadInputDataBit(DHT_GPIO_PORT, DHT_GPIO_PIN)) {
		Delay_us(1);
		if(++timeout > DHT_TIMEOUT_VALUE) return 0; // 超时退出
	}
	
	// 成功接收到DHT11响应
	return 1;
}

/**
  * @brief  接收DHT11发送的一个字节数据
  * @param  无
  * @retval 接收到的8位数据，超时返回0
  */
uint8_t DHT_Get_Byte_Data(void)
{
	uint8_t i;
	uint8_t temp = 0;
	uint16_t timeout = 0;
	
	// 接收8bit数据
	for(i = 0; i < 8; i++)
	{
		// 左移1位，为下一位数据腾出空间
		temp <<= 1;
		
		// 等待50us低电平结束
		timeout = 0;
		while(!GPIO_ReadInputDataBit(DHT_GPIO_PORT, DHT_GPIO_PIN)) {
			Delay_us(1);
			if(++timeout > DHT_TIMEOUT_VALUE) return 0; // 超时返回0
		}
		
		// 延时30us，判断高电平持续时间
		Delay_us(30);
		
		// 如果此时仍为高电平，则为数据'1'，否则为'0'
		if(GPIO_ReadInputDataBit(DHT_GPIO_PORT, DHT_GPIO_PIN)) {
			temp |= 0x01;  // 如果高电平持续时间超过30us，数据为1
			
			// 等待高电平结束
			timeout = 0;
			while(GPIO_ReadInputDataBit(DHT_GPIO_PORT, DHT_GPIO_PIN)) {
				Delay_us(1);
				if(++timeout > DHT_TIMEOUT_VALUE) return 0; // 超时返回0
			}
		}
	}
	
	return temp;
}

/**
  * @brief  获取DHT11的温湿度数据
  * @param  buffer[]: 存储数据的数组，至少5个元素
  * @retval 1:成功 0:失败
  */
uint8_t DHT_Get_Temp_Humi_Data(uint8_t buffer[])
{
	// 清空数据缓冲区
	buffer[0] = buffer[1] = buffer[2] = buffer[3] = buffer[4] = 0;
	
	// 发送起始信号
	if(DHT_Start())
	{
		// 依次接收湿度整数、湿度小数、温度整数、温度小数和校验和
		buffer[0] = DHT_Get_Byte_Data();  // 湿度整数部分
		buffer[1] = DHT_Get_Byte_Data();  // 湿度小数部分
		buffer[2] = DHT_Get_Byte_Data();  // 温度整数部分
		buffer[3] = DHT_Get_Byte_Data();  // 温度小数部分
		buffer[4] = DHT_Get_Byte_Data();  // 校验和
		
		// 验证数据有效性：前四个字节之和等于校验和
		if (buffer[0] + buffer[1] + buffer[2] + buffer[3] == buffer[4]) {
			return 1;  // 数据有效
		}
	}
	
	return 0;  // 通信失败或数据无效
}

/**
  * @brief  初始化DHT11卡尔曼滤波器
  * @param  无
  * @retval 无
  */
void DHT_Filter_Init(void)
{
	uint8_t buffer[5] = {0};
	
	// 读取初始温湿度数据用于滤波器初始化
	if (DHT_Get_Temp_Humi_Data(buffer)) {
		double init_temp, init_humi;
		
		// 温度小数处理
		if (buffer[3] == 0) {
			// 没有小数部分
			init_temp = (double)buffer[2];
		} else {
			// 使用循环除以10的方式处理小数
			init_temp = (double)buffer[2];
			double temp_decimal = buffer[3];
			
			while(temp_decimal > 1) temp_decimal /= 10.0;
			init_temp += temp_decimal;
		}
		
		// 湿度小数处理
		if (buffer[1] == 0) {
			// 没有小数部分
			init_humi = (double)buffer[0];
		} else {
			// 使用循环除以10的方式处理小数
			init_humi = (double)buffer[0];
			double humi_decimal = buffer[1];
			
			while(humi_decimal > 1) humi_decimal /= 10.0;
			init_humi += humi_decimal;
		}
		
		// 温度滤波器初始化
		KalmanFilter_Init(&temp_filter, init_temp, 0.02, 1.0);
		
		// 湿度滤波器初始化
		KalmanFilter_Init(&humi_filter, init_humi, 0.01, 2.0);
		
		is_filter_initialized = 1;
	} else {
		// 读取失败时使用默认值初始化
		KalmanFilter_Init(&temp_filter, 25.0, 0.02, 1.0);  // 默认25℃
		KalmanFilter_Init(&humi_filter, 50.0, 0.01, 2.0);  // 默认50%
	}
}

/**
  * @brief  获取滤波后的温湿度数据
  * @param  buffer[]: 原始数据缓冲区，与DHT_Get_Temp_Humi_Data相同格式
  * @param  filtered_data: 指向存储滤波后数据的结构体
  * @retval 1:成功 0:失败
  */
uint8_t DHT_Get_Filtered_Data(uint8_t buffer[], DHT_FilteredData_t *filtered_data)
{
	// 检查缓冲区数据是否有效
	if (buffer[0] + buffer[1] + buffer[2] + buffer[3] != buffer[4]) {
		return 0;  // 校验和错误
	}
	
	// 如果滤波器未初始化，先初始化
	if (!is_filter_initialized) {
		DHT_Filter_Init();
	}
	
	// 计算原始温度和湿度值
	// 根据小数部分的数值大小，自动确定小数位数
	double raw_temp, raw_humi;
	
	// 温度小数处理
	if (buffer[3] == 0) {
		// 没有小数部分
		raw_temp = (double)buffer[2];
	} else {
		// 使用循环除以10的方式处理小数
		raw_temp = (double)buffer[2];
		double temp_decimal = buffer[3];
		
		while(temp_decimal > 1) temp_decimal /= 10.0;
		raw_temp += temp_decimal;
	}
	
	// 湿度小数处理
	if (buffer[1] == 0) {
		// 没有小数部分
		raw_humi = (double)buffer[0];
	} else {
		// 使用循环除以10的方式处理小数
		raw_humi = (double)buffer[0];
		double humi_decimal = buffer[1];
		
		while(humi_decimal > 1) humi_decimal /= 10.0;
		raw_humi += humi_decimal;
	}
	
	// 应用卡尔曼滤波器
	filtered_data->temperature = KalmanFilter_Update(&temp_filter, raw_temp);
	filtered_data->humidity = KalmanFilter_Update(&humi_filter, raw_humi);
	
	return 1;
}

/**
  * @brief  获取处理后的温湿度数据(简化接口)
  * @param  filtered_data: 指向存储滤波后数据的结构体
  * @retval 1:成功 0:失败
  * @note   此函数封装了数据采集和滤波处理，应用层只需调用此函数即可
  */
uint8_t DHT_GetProcessedData(DHT_FilteredData_t *filtered_data)
{
    uint8_t buffer[5] = {0};
    
    // 如果滤波器未初始化，先初始化
    if (!is_filter_initialized) {
        DHT_Filter_Init();
    }
    
    // 获取原始温湿度数据
    if (DHT_Get_Temp_Humi_Data(buffer)) {
        // 应用卡尔曼滤波处理数据
        return DHT_Get_Filtered_Data(buffer, filtered_data);
    }
    
    return 0; // 采集失败
}

/* 文件结束 -----------------------------------------------------------------*/
