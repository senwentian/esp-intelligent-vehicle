# 硬件参考

## 已支持硬件

| 开发板名  | 主要配置 |
|:--:|:--:|
|ESP-WROVER-KIT V4.1|ESP32-WROVER-B + OV2640 + CAN收发器 + 反相器 + 接收机|



### 基础配置

#### 基础配置清单

| 基础配置清单  | 数量 | 备注 |
|:--:|:--:|:--:|
|主板|1|ESP32-WROVER-B + OV2640|
|直流无刷减速电机|4||
|电子调速器 |4 | 通过CAN总线对电机进行调速控制 |
|电调中心板|１|将一路24v电源分为4路供给四个电机|
|58mm全向轮|４||
|5300mAh 6s 锂电池|1|24v供电|
|舵机|２|控制机械臂运动角度|
|CAN收发器|１|连接芯片集成的TWAI控制器，将二进制流转换为差分信号|
|反相器|１|反相SBUS协议的电平逻辑（可选）|
|接收机|１|接受SBUS信号，其信号线通过反相器接入开发板的Rx（可选）|



#### 主控制器

| 芯片型号  | 模组型号 | 备注 |
|--|--|--|
| ESP32 | ESP32-WROVER-B | 模组内置 4 MB flash，8 MB PSRAM |



#### 指示灯

| 状态 | LED | 动作 |
|:--|:-:|:-|
|Power_ON|RED|常亮|
|Waiting for network distribution|GREEN|慢速闪烁（2Hz）|
|Waiting for TCP Client to initiate a connection|GREEN|快速闪烁（10Hz）|



#### 按键

| 按键 | IO | 功能 |
|--|--|--|
|SW1|GPIO0|短按开启OTA，长按3秒erase_flash|
|SW2|EN|Reset|



#### 主板 IO 定义

| 管脚 | 功能 | 备注 |
| :---: | :---: | :---: |
| GPIO0 | LED_RED | 摄像头RESET复用 |
| GPIO2 |   LED_GREEN、PWM1   | 1号舵机 |
| GPIO4 | LED_BLUE | 摄像头 D0 复用 |
| GPIO12 | PWM2 |2号舵机 |
| GPIO13 | UART_Rx |接收机信号线|
| GPIO14 | TWAI_Tx |CAN收发器 |
| GPIO15 | TWAI_Rx |CAN收发器|
| GPIO16 | 连接模组集成的PSRAM ||
| GPIO17 | 连接模组集成的PSRAM |  |



#### 摄像头接口

| 管脚 | 功能 |
| :---: | :---: |
|GPIO27  | SIO_C / SCCB 时钟 |
|GPIO26  | SIO_D / SCCB 数据 |
|GPIO25  | VSYNC / 垂直同步 |
|GPIO23  | HREF / 水平参考 |
|GPIO22  |PCLK / 像素时钟  |
|GPIO21  |XCLK / 系统时钟  |
|GPIO35  |D7 / 像素数据 Bit 7  |
|GPIO34  | D6 / 像素数据 Bit 6 |
|GPIO39  | D5 / 像素数据 Bit 5 |
|GPIO36  |D4 / 像素数据 Bit 4  |
|GPIO19 | D3 / 像素数据 Bit 3 |
|GPIO18  |D2 / 像素数据 Bit 2  |
|GPIO5 |D1 / 像素数据 Bit 1 |
|GPIO4 |D0 / 像素数据 Bit 0 |
|GPIO0 |RESET / 摄像头复位 |

