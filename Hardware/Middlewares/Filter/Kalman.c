/**
  ******************************************************************************
  * @file    kalman.c
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   卡尔曼滤波器驱动实现
  ******************************************************************************
  */

/* 包含头文件 ----------------------------------------------------------------*/
#include "kalman.h"

/**
  * @brief  初始化卡尔曼滤波器
  * @param  filter: 指向卡尔曼滤波器结构体
  * @param  init_value: 初始值
  * @param  Q: 过程噪声协方差
  * @param  R: 测量噪声协方差
  * @retval 无
  */
void KalmanFilter_Init(KalmanFilter_t *filter, double init_value, double Q, double R)
{
    /* 初始化滤波器状态 */
    filter->x = init_value;  // 初始估计值
    filter->Q = Q;           // 过程噪声协方差
    filter->R = R;           // 测量噪声协方差
    filter->P = 1.0;         // 初始估计误差协方差
    filter->K = 0.0;         // 初始卡尔曼增益
}

/**
  * @brief  更新滤波器并返回滤波后的值
  * @param  filter: 指向卡尔曼滤波器结构体
  * @param  measurement: 当前测量值
  * @retval 滤波后的值
  */
double KalmanFilter_Update(KalmanFilter_t *filter, double measurement)
{
    /* 预测更新阶段 */
    // 由于使用常数模型，预测状态不变，只需增加过程噪声
    filter->P = filter->P + filter->Q;
    
    /* 测量更新阶段 */
    // 计算卡尔曼增益
    filter->K = filter->P / (filter->P + filter->R);
    
    // 更新估计值
    filter->x = filter->x + filter->K * (measurement - filter->x);
    
    // 更新估计误差协方差
    filter->P = (1 - filter->K) * filter->P;
    
    /* 返回滤波后的值 */
    return filter->x;
}

/* 文件结束 -----------------------------------------------------------------*/
