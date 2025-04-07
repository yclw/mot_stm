/**
  ******************************************************************************
  * @file    kalman.h
  * @author  农业大棚监控小组
  * @version V1.0
  * @date    2024-03-07
  * @brief   卡尔曼滤波器驱动头文件
  ******************************************************************************
  */

#ifndef INC_KALMAN_FILTER_H_
#define INC_KALMAN_FILTER_H_

/* 类型定义 ------------------------------------------------------------------*/
/**
  * @brief  卡尔曼滤波器结构体
  */
typedef struct {
    double Q;    // 过程噪声协方差
    double R;    // 测量噪声协方差
    double x;    // 状态估计值（滤波后的值）
    double P;    // 估计误差协方差
    double K;    // 卡尔曼增益
} KalmanFilter_t;

/* 函数声明 ------------------------------------------------------------------*/
/**
  * @brief  初始化卡尔曼滤波器
  * @param  filter: 指向卡尔曼滤波器结构体
  * @param  init_value: 初始值
  * @param  Q: 过程噪声协方差
  * @param  R: 测量噪声协方差
  * @retval 无
  */
void KalmanFilter_Init(KalmanFilter_t *filter, double init_value, double Q, double R);

/**
  * @brief  更新滤波器并返回滤波后的值
  * @param  filter: 指向卡尔曼滤波器结构体
  * @param  measurement: 当前测量值
  * @retval 滤波后的值
  */
double KalmanFilter_Update(KalmanFilter_t *filter, double measurement);

#endif /* INC_KALMAN_FILTER_H_ */

/* 文件结束 -----------------------------------------------------------------*/
