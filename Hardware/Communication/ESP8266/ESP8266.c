/**
  ******************************************************************************
  * @file    ESP8266.c
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   ESP8266 WiFi模块驱动程序
  ******************************************************************************
  */

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f10x.h"                  // 设备头文件
#include "string.h"                     // 字符串处理
#include "Delay.h"                      // 延时函数
#include "ESP8266.h"                    // ESP8266接口
#include <stdio.h>                      // 标准输入输出
#include "stdint.h"                     // 标准整型
#include <stdarg.h>                     // 可变参数

/* 私有定义 ------------------------------------------------------------------*/
#define USART1_RX_BUFFER_SIZE 256       // 接收缓冲区大小
#define ESP8266_TIMEOUT        1000     // 通用超时时间(ms)
#define ESP8266_MAX_RETRIES    3        // 最大重试次数

/* 私有变量 ------------------------------------------------------------------*/
volatile uint8_t  USART1_RxBuffer[USART1_RX_BUFFER_SIZE]; // 接收缓冲区
volatile uint16_t USART1_RxHead = 0;    // 缓冲区头指针
volatile uint16_t USART1_RxTail = 0;    // 缓冲区尾指针

/* 私有函数声明 --------------------------------------------------------------*/
static int ESP8266_WaitForResponse(void);

/* 串口通信模块 --------------------------------------------------------------*/

/**
  * @brief  串口1初始化
  * @param  无
  * @retval 无
  */
void Serial_Init(void)
{
    /* 开启外设时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);  // USART1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // GPIOA时钟
	
    /* GPIO初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // PA9: USART1_TX，复用推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    // PA10: USART1_RX，上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    /* USART初始化 */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;                // 波特率
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  // 无硬件流控
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 收发模式
    USART_InitStructure.USART_Parity = USART_Parity_No;         // 无校验
    USART_InitStructure.USART_StopBits = USART_StopBits_1;      // 1位停止位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; // 8位数据位
    USART_Init(USART1, &USART_InitStructure);
	
    /* 中断配置 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  // 使能接收中断
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 中断分组
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
	
    /* 使能USART */
    USART_Cmd(USART1, ENABLE);
}

/**
  * @brief  串口发送一个字节
  * @param  Byte: 要发送的字节
  * @retval 无
  */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // 等待发送完成
}

/**
  * @brief  重定向printf底层函数
  * @param  ch: 要发送的字符
  * @param  f: 文件指针（此处未使用）
  * @retval 发送的字符
  */
int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);
    return ch;
}

/**
  * @brief  USART1中断处理函数
  * @param  无
  * @retval 无
  * @note   接收数据并存储到环形缓冲区
  */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t data = USART_ReceiveData(USART1);
        uint16_t nextHead = (USART1_RxHead + 1) % USART1_RX_BUFFER_SIZE;
        
        // 检查缓冲区是否已满
        if (nextHead != USART1_RxTail)
        {
            USART1_RxBuffer[USART1_RxHead] = data;
            USART1_RxHead = nextHead;
        }
    }
}

/**
  * @brief  从环形缓冲区中读取一个字节
  * @param  data: 读取数据的存放地址
  * @retval 1:成功读取 0:缓冲区为空
  */
int USART1_ReadByte(uint8_t *data)
{
    if (USART1_RxHead == USART1_RxTail)
        return 0; // 缓冲区为空
    
    *data = USART1_RxBuffer[USART1_RxTail];
    USART1_RxTail = (USART1_RxTail + 1) % USART1_RX_BUFFER_SIZE;
    
    return 1;
}

/* ESP8266模块功能实现 -------------------------------------------------------*/

/**
  * @brief  等待并检查ESP8266响应
  * @param  无
  * @retval 1:收到OK响应 0:超时或未收到期望响应
  */
static int ESP8266_WaitForResponse(void)
{
    uint8_t data;
    uint8_t response[20] = {0};
    uint8_t index = 0;
    uint16_t timeout = 0;
    
    while (timeout < ESP8266_TIMEOUT)
    {
        if (USART1_ReadByte(&data))
        {
            response[index++] = data;
            if (index >= sizeof(response) - 1)
                index = 0; // 防止缓冲区溢出
                
            // 检查响应是否包含"OK"
            if (index >= 2 && response[index-2] == 'O' && response[index-1] == 'K')
                return 1;
        }
        else
        {
            timeout++;
            Delay_ms(1);
        }
    }
    
    return 0; // 超时未收到OK
}

/**
  * @brief  初始化ESP8266
  * @param  无
  * @retval 无
  */
void ESP8266_Init(void)
{
    /* 初始化串口 */
    Serial_Init();
    
    uint8_t retryCount = 0;
    uint8_t cmdIndex = 0;
    uint8_t success = 0;
    
    /* 清空接收缓冲区 */
    USART1_RxHead = USART1_RxTail = 0;
    
    /* 配置命令列表 */
    const char *commands[] = {
        "AT\r\n",                                       // 测试AT指令
        "AT+CIPSTART=\"TCP\",\"117.72.118.76\",3000\r\n", // 建立TCP连接
        "AT+CIPMODE=1\r\n",                             // 透传模式
        "AT+CIPSEND\r\n"                                // 开始透传
    };
    uint8_t cmdCount = 4; // 命令数量
    
    /* 尝试执行AT命令序列 */
    while (retryCount < ESP8266_MAX_RETRIES && !success)
    {
        cmdIndex = 0;
        success = 1;
        
        while (cmdIndex < cmdCount)
        {
            // 发送命令
            printf("%s", commands[cmdIndex]);
            
            // 等待响应
            Delay_ms(1000);
            
            // 检查响应
            if (!ESP8266_WaitForResponse())
            {
                success = 0;
                break;
            }
            
            cmdIndex++;
        }
        
        if (!success)
        {
            retryCount++;
            Delay_ms(2000);
        }
    }
}

/**
  * @brief  重启ESP8266
  * @param  无
  * @retval 无
  */
void ESP8266_Restart(void)
{
    printf("+++");            // 退出透传模式
    Delay_ms(500);
    printf("AT+RST\r\n");     // 发送重启命令
    Delay_ms(3000);           // 等待重启完成
    ESP8266_Init();           // 重新初始化
}

/**
  * @brief  发送HTTP POST请求
  * @param  POST: POST请求路径
  * @param  Host: 主机地址和端口
  * @param  json: JSON格式的数据
  * @retval 1:发送成功 0:发送失败
  */
int ESP8266_Send_http_post(char *POST, char *Host, char *json)
{
    int length = strlen(json);
    int ret = printf("POST %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Connection: keep-alive\r\n"
                     "User-Agent: ESP8266\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %d\r\n"
                     "\r\n"
                     "%s\r\n",
                     POST, Host, length, json);
    return (ret > 0) ? 1 : 0;
}

/**
  * @brief  接收HTTP响应并解析状态码
  * @param  code: 解析出的HTTP状态码存放地址
  * @retval 1:接收并解析成功 0:接收失败或解析失败
  */
int ESP8266_Receive_http_response(uint32_t *code)
{
    char response_buffer[256] = {0};
    uint16_t index = 0;
    char ch;
    uint16_t noDataCounter = 0;
    const uint16_t maxNoDataCount = 15000;  // 无数据超时阈值
    
    /* 接收HTTP响应 */
    while (noDataCounter < maxNoDataCount)
    {
        if (USART1_ReadByte((uint8_t *)&ch))
        {
            response_buffer[index++] = ch;
            noDataCounter = 0;  // 收到数据时重置计数器
            
            // 防止缓冲区溢出
            if (index >= sizeof(response_buffer) - 1) {
                break;
            }
            
            // 检查是否已接收到HTTP头结束标志
            if (index >= 4 && strstr(response_buffer, "\r\n\r\n") != NULL) {
                break;
            }
        }
        else
        {
            noDataCounter++;
        }
    }
    response_buffer[index] = '\0';  // 字符串结束标志
    
    /* 解析HTTP状态码 */
    char *http_header = strstr(response_buffer, "HTTP/1.1 ");
    if (!http_header) return 0;
    
    char *status_start = http_header + 9;  // 指向状态码起始位置
    if (sscanf(status_start, "%3u", code) != 1) {
        return 0;
    }
    
    return 1;
}

/* 文件结束 -----------------------------------------------------------------*/
