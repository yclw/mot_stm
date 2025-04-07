# 农业大棚环境监测系统

## 项目简介
本项目是一个基于STM32的农业大棚环境监测系统，实现了温湿度、光照等环境参数的实时监测、显示和远程上传功能。系统采用模块化设计，具有良好的可扩展性和可维护性。

## 编码说明
> **重要提示**：本项目所有源代码文件均使用UTF-8编码。如果打开文件时出现中文注释乱码，请：
> 1. 在编辑器中设置文件编码为UTF-8
> 2. 如果仍然无法正确显示，建议在GitHub上直接查看代码和注释
> 3. 对于Keil MDK用户，请在Options->Editor->Encoding中设置为UTF-8

## 项目特点
1. **模块化设计**
   - 采用分层架构，将驱动层、应用层和配置层分离
   - 各模块之间低耦合，便于维护和扩展
   - 代码结构清晰，注释规范

2. **数据可靠性**
   - 使用卡尔曼滤波算法处理传感器数据
   - 实现了传感器错误检测和自动恢复机制
   - 网络通信具有重试和错误处理机制

3. **实时监控**
   - OLED实时显示环境参数
   - 异常情况蜂鸣器报警
   - 数据实时上传至服务器

## 系统架构

### 1. 目录结构
```
├── Start/                    # 启动文件目录
│
├── System/                   # 系统核心目录
│   ├── Delay.h               # 延时函数头文件
│   └── Delay.c               # 延时函数实现
│
├── User/                     # 用户代码目录
│   ├── App/                  # 应用层代码
│   │   ├── app.h             # 应用层头文件
│   │   └── app.c             # 应用层实现文件
│   │
│   ├── Config/               # 配置文件目录
│   │   └── config.h          # 系统配置文件
│   │
│   ├── stm32f10x_it.c        # 中断处理函数
│   ├── stm32f10x_it.h        # 中断处理头文件
│   ├── stm32f10x_conf.h      # 外设配置头文件
│   └── main.c                # 主程序入口
│
├── Hardware/                 # 硬件驱动目录
│   ├── README.md             # 硬件模块说明文档
│   ├── Actuator/             # 执行器驱动
│   │   └── buzzer/           # 蜂鸣器驱动
│   │
│   ├── Sensor/               # 传感器驱动
│   │   ├── dht11/            # DHT11温湿度传感器
│   │   └── light/            # 光照传感器
│   │
│   ├── Display/              # 显示模块驱动
│   │   └── oled/             # OLED显示驱动
│   │
│   ├── Communication/        # 通信模块驱动
│   │   └── esp8266/          # ESP8266 WiFi模块
│   │
│   └── Middlewares/          # 中间件
│       └── kalman/           # 卡尔曼滤波算法
│
├── MDK-ARM/                  # Keil MDK工程目录
│
├── .gitignore                # Git忽略文件
├── LICENSE                   # 许可证文件
└── README.md                 # 项目说明文档
```

### 2. 代码架构

#### 2.1 应用层 (User/App/)
- **app.h/app.c**: 应用层核心代码
  - `App_Init()`: 系统初始化
  - `App_MainLoop()`: 主循环处理
  - `App_ProcessSensorData()`: 传感器数据处理
  - `App_HandleSensorError()`: 错误处理
  - `App_UploadData()`: 数据上传

#### 2.2 配置层 (User/Config/)
- **config.h**: 系统配置参数
  - 系统参数：主循环延时、显示参数等
  - 网络参数：服务器地址、API路径等

#### 2.3 驱动层 (User/Drivers/)
- **oled.h/oled.c**: OLED显示驱动
- **dht11.h/dht11.c**: 温湿度传感器驱动
- **light.h/light.c**: 光照传感器驱动
- **esp8266.h/esp8266.c**: WiFi通信驱动
- **buzzer.h/buzzer.c**: 蜂鸣器驱动

## 代码详解

### 1. 主程序 (main.c)
```c
int main(void)
{
    /* 系统初始化 */
    App_Init();
    
    /* 主循环 */
    while (1)
    {
        App_MainLoop();
    }
}
```
主程序结构简洁，只负责初始化和主循环调度。

### 2. 应用层实现 (app.c)

#### 2.1 系统初始化
```c
void App_Init(void)
{
    /* 初始化OLED显示 */
    OLED_Init();
    OLED_ShowString(1, 1, "System Init...");

    /* 初始化各模块 */
    Light_Init();
    DHT_Filter_Init();
    ESP8266_Init();
    Buzzer_Init();

    /* 延时确保传感器稳定 */
    Delay_ms(100);
    OLED_Clear();
}
```

#### 2.2 主循环处理
```c
void App_MainLoop(void)
{
    /* 处理传感器数据 - 数据采集和处理已封装在传感器驱动层 */
    App_ProcessSensorData();

    Delay_ms(MAIN_LOOP_DELAY_MS);
}
```

#### 2.3 传感器数据处理
```c
void App_ProcessSensorData(void)
{
    DHT_FilteredData_t filtered_data;
    
    /* 获取传感器数据 (使用简化接口) */
    if (!DHT_GetProcessedData(&filtered_data)) {
        App_HandleSensorError();
        return;
    }
    
    /* 重置错误计数 */
    dht_error_count = 0;
    
    /* 获取光照值 */
    uint16_t light = Light_Get();
    
    /* 数据显示 */
    sprintf(tempDisplayStr, "T:%.1lfC", filtered_data.temperature);
    sprintf(humiDisplayStr, "H:%.1lf%%", filtered_data.humidity);
    sprintf(lightDisplayStr, "Lux:%4d", light);
    
    /* 更新显示 */
    OLED_ShowString(1, 1, lightDisplayStr);
    OLED_ShowString(2, 1, tempDisplayStr);
    OLED_ShowString(3, 1, humiDisplayStr);
    
    /* 报警控制 */
    Buzzer_Control(light, (int)filtered_data.temperature, (int)filtered_data.humidity);
    
    /* 数据上传 */
    App_UploadData(filtered_data.temperature, filtered_data.humidity, light);
}
```

### 3. 错误处理机制
```c
void App_HandleSensorError(void)
{
    dht_error_count++;
    
    /* 显示错误信息 */
    sprintf(errMsg, "Sen err %d/%d    ", dht_error_count, MAX_ERROR_COUNT);
    OLED_ShowString(2, 1, errMsg);
    
    /* 错误恢复 */
    if (dht_error_count >= MAX_ERROR_COUNT)
    {
        OLED_ShowString(2, 1, "Reinit sensor   ");
        Delay_ms(1000);
        dht_error_count = 0;
    }
}
```

### 4. 网络通信
```c
void App_UploadData(float temperature, float humidity, uint16_t light)
{
    /* 数据上传条件检查 */
    if (network_error_count == 0 || 
        (current_time - last_successful_time > NETWORK_RETRY_INTERVAL))
    {
        /* 数据打包 */
        sprintf(json, "{\"temperature\": %.1f, \"humidity\": %.1f, \"light\": %d}",
                temperature, humidity, light);

        /* 发送数据 */
        if (ESP8266_Send_http_post(POST_PATH, SERVER_HOST, json))
        {
            /* 处理响应 */
            if (ESP8266_Receive_http_response(&code))
            {
                /* 成功处理 */
                last_successful_time = current_time;
                network_error_count = 0;
            }
            else
            {
                /* 错误处理 */
                network_error_count++;
                if (network_error_count >= MAX_ERROR_COUNT)
                {
                    ESP8266_Restart();
                    network_error_count = 0;
                }
            }
        }
    }
}
```

## 硬件模块说明
详细硬件模块说明请参考 [Hardware/README.md](Hardware/README.md)

## 使用说明

### 1. 开发环境搭建
1. **安装开发工具**
   - 下载并安装Keil MDK-ARM (建议版本5.36或更高)
   - 安装STM32F1xx_DFP支持包
   - 安装ARM Compiler V5.06或更高版本

2. **配置开发环境**
   - 设置编译器为ARM Compiler
   - 配置工程包含路径
   - 设置输出文件格式为HEX

### 2. 硬件准备
1. **所需硬件**
   - STM32F103C8T6开发板
   - OLED显示屏 (I2C接口)
   - DHT11温湿度传感器
   - 光照传感器 (ADC接口)
   - ESP8266 WiFi模块
   - 蜂鸣器模块
   - USB转串口模块
   - 杜邦线若干

2. **硬件连接**
   - 按照[硬件模块说明](Hardware/README.md)中的引脚定义进行连接
   - 确保所有模块供电正常
   - 检查各接口连接是否牢固

### 3. 软件配置
1. **工程配置**
   - 打开工程文件 `MDK-ARM/Project.uvprojx`
   - 检查目标芯片型号是否正确
   - 配置系统时钟为72MHz
   - 设置调试接口为SWD

2. **网络配置**
   - 修改 `User/Config/config.h` 中的服务器地址
   - 配置WiFi模块的SSID和密码
   - 设置数据上传间隔

### 4. 编译和下载
1. **编译工程**
   - 点击"Rebuild"按钮编译整个工程
   - 检查编译输出，确保无错误和警告
   - 生成HEX文件

2. **程序下载**
   - 连接开发板和电脑
   - 选择正确的下载器
   - 点击"Download"按钮下载程序
   - 确认下载成功

### 5. 运行和调试
1. **系统启动**
   - 上电后观察OLED显示
   - 检查各模块初始化状态
   - 确认传感器数据采集正常

2. **功能测试**
   - 观察温湿度数据显示
   - 检查光照值变化
   - 验证网络连接状态
   - 测试报警功能

3. **问题排查**
   - 使用串口调试工具查看日志
   - 检查各模块供电情况
   - 验证通信接口连接
   - 查看错误提示信息

## 注意事项
1. 确保传感器连接正确
2. 检查WiFi模块配置
3. 注意供电稳定性
4. 定期检查传感器状态

## 未来改进
1. 添加更多传感器支持
2. 实现本地数据存储
3. 优化通信协议
4. 添加远程控制功能
