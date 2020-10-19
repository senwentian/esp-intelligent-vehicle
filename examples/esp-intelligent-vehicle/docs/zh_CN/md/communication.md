## 通信层级结构

|||||
|:--:|:--:|:--:|:--:|
|终端|PC|ESP-Intelligent-Vehicle|遥控器|
|应用层|NetCat | 固件 | |
|协议层|IP |IP |SBUS |
|传输层|TCP|TCP||
|物理层|Wi-Fi STA (Station)|Wi-Fi STA (Station)||

> 用户可以通过在`menuconfig`中选择控制模式来进行对小车的控制
>
> １.通过扫描二维码配网，使PC和ESP-Intelligent-Vehicle处于同一个Wi-Fi局域网之内，通过建立TCP连接来使用户控制该智能小车
>
> ２.通过支持SBUS通讯协议的遥控器来对该智能小车进行控制

## Wi-Fi 通信

### ESP32 Wi-Fi 性能

 **ESP32 Wi-Fi 性能**

| 项目 | 参数 |
|--|--|
| 模式 | STA 模式、AP 模式、共存模式 |
| 协议 | IEEE 802.11b、IEEE 802.11g、IEEE 802.11n、802.11 LR（乐鑫）支持软件切换 |
| 安全性 |WPA、WPA2、WPA2-Enterprise、WPS |
| 主要特性 |AMPDU、HT40、QoS |
| 支持距离 |乐鑫专属协议下 1 km|
| 传输速率 |20 Mbit/s TCP 吞吐量、30 Mbit/s UDP |

其它参数见 [ESP32 Wi-Fi 特性列表](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#esp32-wi-fi-feature-list)。

>ESP-Intelligent-Vehicle设置为STA模式，通过摄像头扫描用户所连接到的AP，获取到信息之后连接至该AP

### Wi-Fi 编程框架

**基于 ESP-IDF 的 Wi-Fi 编程框架：**

![Wi-Fi 编程模型](https://img-blog.csdnimg.cn/20200423173923300.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzIwNTE1NDYx,size_16,color_FFFFFF,t_70#pic_center)

**一般使用过程如下：**

1. 应用层调用 [Wi-Fi 驱动 API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)，进行 Wi-Fi 初始化。
2. Wi-Fi 驱动对开发人员透明。事件发生，则 Wi-Fi 驱动向默认事件循环：[default event loop](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_event.html#esp-event-default-loops)
发布 `event`。应用程序可根据需求编写 `handle` 程序，进行注册。
3. 网络接口组件 [esp_netif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_netif.html) 提供了一系列 `handle` 程序，与 Wi-Fi 驱动 `event` 默认关联。例如 ESP32 作为 STA模式，当解析到AP的ssid和password等信息之后，调用`esp_wifi_connect()`API进行Wi-Fi连接。

具体的使用过程，可查阅代码 `example\intelligent-vehicle\components\wifi\app_wifi.c`。

注意：Wi-Fi 初始化之前应使用 `WIFI_INIT_CONFIG_DEFAULT` 获取初始化配置结构体，对该结构体进行个性化配置，然后进行初始化工作。请注意防范结构体成员未初始化导致的问题，在 ESP-IDF 更新添加了新的结构体成员时，应尤其特别注意这一问题。

**STA 模式工作状态图:**

![Sample Wi-Fi Event Scenarios in STA Mode](https://docs.espressif.com/projects/esp-idf/en/stable/_images/seqdiag-8cc01a9d7d1eef82ac3cd33e6c75000b96a78210.png)

### 提高 Wi-Fi 通信距离

通过`menuconfig`依次进入：`Component config>>PHY>>Max WiFi TX power (dBm)`，将 `Max WiFi TX power` 改为 `20`。该项配置将提高 PHY 增益，提高 Wi-Fi 通信距离。

## TCP 通信

### TCP 端口号

| PC |方向 | ESP-Intelligent-Vehicle |
|--|--|--|
| 192.168.0.162::3333 | TX/RX | 192.168.0.162::3333 |



### TCP 包结构

![TCP 包结构](https://img2018.cnblogs.com/blog/922925/201906/922925-20190606100850630-692620940.png)



### 如何使PC端与ESP-Intelligent-Vehicle进行通信

ESP-Intelligent-Vehicle通过使用`menuconfig`中预定义的端口号创建一个TCP套接字，并**等待**来自客户端的连接请求。接受来自客户端的请求后，**服务器与客户端之间将建立连接**，之后系统应用程序任务开始执行。其中，TCP_Server_Task将等待从客户端接收一些命令，接收到的命令经**解析后执行相应的操作**并向客户端发送**应答消息**。

为了在PC端创建与ESP-Intelligent-Vehicle通信的TCP Client，可以使用可用于与UDP / TCP、服务器/客户端进行交互的命令行工具[netcat](http://netcat.sourceforge.net/)，其可以发送和接收多种数据包。

> 注意：请在以下命令中用所需的IPV4 / IPV6地址和端口号进行替换。

**使用netcat的TCP客户端**

```
nc 192.168.0.167 3333
```

注意：一定要在系统启动之后观察到开发板上的LED开始闪烁绿灯，再进行PC端的TCP Client连接。



## SBUS通信

> 在遥控器模式下，遥控器通过SBUS协议对ESP-Intelligent-Vehicle进行控制

### 协议介绍

**S.BUS**是FUTABA提出的舵机控制总线，全称Serial Bus，别名S-BUS或SBUS，也称 [Futaba S.BUS](https://www.futabarc.com/sbus/index.html)。
S.BUS是一个串行通信协议，也是一个数字串行通信接口（单线），适合与飞控连接。它可以连接很多设备，每个设备通过一个HUB与它相连，得到各自的控制信息。
S.BUS可以传输**16**个比例通道和**2**个数字（bool）通道。其硬件上基于RS232协议，采用TTL电平，但高位取反（负逻辑，低电平为“1”，高电平为“0”），通信波特率为100K（不兼容波特率115200）。

### 协议解析

1. 通信接口：UART（TTL）
2. 通信参数：1个起始位+8个数据位+偶校验位+2个停止位，波特率=**100000**bit/s，电平逻辑反转。
3. 通信速率：每14ms（模拟模式）或7ms（高速模式）发送，即数据帧间隔为 11ms（模拟模式）或4ms（高速模式）。
4. 数据帧格式：

| 字节位 | byte1    | byte2-23                       | byte24                      | byte25   |
| ------ | -------- | ------------------------------ | --------------------------- | -------- |
| 类型   | 开始字节 | 通道数据字节（含16个脉宽通道） | 标志位字节（含2个数字通道） | 结束字节 |
| 数据   | 0x0F     | 通道数据范围11Bits = [0,2047]  | 2个数字通道位+2个状态位     | 0x00     |

**byte1:**
startbyte = 0000 1111b (0x0F)

**byte2-23:**
databytes = 22bytes = 22 x 8Bits = 16 x 11Bits(CH1-16)
通道数据低位在前，高位在后，每个数据取11位，具体协议如下：
读取的databyte值：

| byte | 2        | 3          | 4        | 5            | 6        | 7          | etc  |
| ---- | -------- | ---------- | -------- | ------------ | -------- | ---------- | ---- |
| 内容 | 12345678 | `12345678` | 12345678 | **12345678** | 12345678 | `12345678` | etc  |

转化后的通道值：

| 通道 | CH01          | CH02          | CH03            | CH04          | etc  |
| ---- | ------------- | ------------- | --------------- | ------------- | ---- |
| 内容 | `678`12345678 | 345678`12345` | 8**12345678**12 | `5678`1234567 | etc  |

**byte24:**

| Bit  | 7            | 6            | 5        | 4              | 3    | 2    | 1    | 0    |
| ---- | ------------ | ------------ | -------- | -------------- | ---- | ---- | ---- | ---- |
| 含义 | 数字通道CH17 | 数字通道CH18 | 帧丢失位 | 故障保护激活位 | N/A  | N/A  | N/A  | N/A  |

**byte25:**
endbyte = 0000 0000b (0x00)



### 硬件连接

接收机信号线连接开发板的GPIO_NUM_13 (Rx)，因为sbus协议传输的信号为TTL反向电平，所以需要在开发板的GPIO口与接收机信号线之间连接一个反向器（74HC04D）使芯片接收到正确的TTL电平信息。



### 程序设计

```c
switch(event.type) {
    //Event of UART receving data
    /*We'd better handler data event fast, there would be much more data events than
    other types of events. If we take too much time on data event, the queue might
    be full.*/
    case UART_DATA:
        read_len = uart_read_bytes(UART_NUM_1, dtmp, event.size, portMAX_DELAY);
        dtmp[read_len] = 0;
        const uint8_t *d = dtmp;
        static uint8_t RxState = 0, RxDataIndex = 0;
        while(flag){
            switch(RxState){
                case 0:				//The start mark has not been detected yet, start to match the start mark
                    if(*d == 0x0F)
                    {
                        RxState++;
                        SBUS_MsgPack[0] = *d;
                        RxDataIndex = 1;
                    }
                    else
                        RxState = 0;
                    break;
                case 1:					//Start mark matches, start to receive raw data
                    SBUS_MsgPack[RxDataIndex] = *d;
                    RxDataIndex++;
                    if(RxDataIndex >= 23)
                    {
                        RxDataIndex = 0;
                        RxState++;
                    }
                    break;
                case 2:					//The data has been received, the first byte of the start matching end flag
                        SBUS_MsgPack[23] = *d;
                        RxState++;
                    break;
                case 3:					//The first byte of the end flag is matched, and the second byte of the start matching end flag
                    if(*d == 0x00)
                    {
                        UpdateRemoteInfo((void*)&SBUS_MsgPack[0]);
                        SBUS_MsgPack[24] = 0x00;
                        RemoteUpdated = 1;
                    }
                    RxState = 0;
                    flag = 0;
                    break;
                default:
                    RxState = 0;
            }
            d++;
        }
        //ESP_LOGI("UART_Test", "Remote.Rx: %d, %d, %d, %d, %d, %d", SBUS_ChanelVal[0], SBUS_ChanelVal[1], SBUS_ChanelVal[2], SBUS_ChanelVal[4], SBUS_ChanelVal[5], SBUS_ChanelVal[6]);
        break;

    default:
        // ESP_LOGI("UART_Test", "uart event type: %d", event.type);
        break;
    }
```

以上是uart_recieve_task()任务接受sbus数据帧的部分代码

根据协议解析，通过switch case来匹配起始字节和结束字节，只有起始字节和结束字节分别符合协议规定的0x0F和0x00，才能进一步对接收到的数据帧进行协议转换

```c
void SBUS_Decode(void)
{
	SBUS_ChanelVal[0]  = ((SBUS_MsgPack[1]		| SBUS_MsgPack[2] << 8)	& 0x07FF);
	SBUS_ChanelVal[1]  = ((SBUS_MsgPack[2] >> 3	| SBUS_MsgPack[3] << 5)	& 0x07FF);
	SBUS_ChanelVal[2]  = ((SBUS_MsgPack[3] >> 6	| SBUS_MsgPack[4] << 2 | SBUS_MsgPack[5] << 10)	& 0x07FF);
	SBUS_ChanelVal[3]  = ((SBUS_MsgPack[5] >> 1	| SBUS_MsgPack[6] << 7)	& 0x07FF);
	SBUS_ChanelVal[4]  = ((SBUS_MsgPack[6] >> 4	| SBUS_MsgPack[7] <<4)	& 0x07FF);
	SBUS_ChanelVal[5]  = ((SBUS_MsgPack[7] >> 7	| SBUS_MsgPack[8] << 1 | SBUS_MsgPack[9] << 9)	& 0x07FF);
	SBUS_ChanelVal[6]  = ((SBUS_MsgPack[9] >> 2	| SBUS_MsgPack[10] << 6)	& 0x07FF);
	SBUS_ChanelVal[7]  = ((SBUS_MsgPack[10] >> 5	| SBUS_MsgPack[11] << 3)	& 0x07FF);
	SBUS_ChanelVal[8]  = ((SBUS_MsgPack[12]		| SBUS_MsgPack[13] << 8)	& 0x07FF);
	SBUS_ChanelVal[9]  = ((SBUS_MsgPack[13] >> 3	| SBUS_MsgPack[14]<<5)	& 0x07FF);
	SBUS_ChanelVal[10] = ((SBUS_MsgPack[14] >> 6	| SBUS_MsgPack[15]<<2 | SBUS_MsgPack[16] << 10)	& 0x07FF);
	SBUS_ChanelVal[11] = ((SBUS_MsgPack[16] >> 1	| SBUS_MsgPack[17]<<7)	& 0x07FF);
	SBUS_ChanelVal[12] = ((SBUS_MsgPack[17] >> 4	| SBUS_MsgPack[18]<<4)	& 0x07FF);
	SBUS_ChanelVal[13] = ((SBUS_MsgPack[18] >> 7	| SBUS_MsgPack[19]<<1 | SBUS_MsgPack[20]<<9)	& 0x07FF);
	SBUS_ChanelVal[14] = ((SBUS_MsgPack[20] >> 2	| SBUS_MsgPack[21]<<6)	& 0x07FF);
	SBUS_ChanelVal[15] = ((SBUS_MsgPack[21] >> 5	| SBUS_MsgPack[22]<<3)	& 0x07FF);
}
```



具体请参考`example/intelligent-vehicle/component/Peripherals/user_uart.c`以及``example/intelligent-vehicle/component/drive-control/user_sbus.c`



－－－－－－－－－－－－－－－－－

未完待续

