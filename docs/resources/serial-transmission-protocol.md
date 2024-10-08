<!-- markdownlint-disable-file -->

# 搭载板图像加速处理软件与中心机 串口传输协议

## 概述

搭载板图像加速处理软件与中心机之间的通信通过 RS422 串口实现。中心机向搭载版传输 3840\*2160pix 的图像数据，图像格式为 YUV(422)；搭载板对图像数据进行处理；处理完成后，搭载版向中心机传输二进制熵编码码流。此文档为中心机和搭载板之间通信的具体规约。

## 串口工作原理

### UART 控制器框图

UART 控制器框图如图 2-1 所示。

![图2-1 UART控制器](https://github.com/ustc-ivclab/.github/assets/32936898/3fbf035a-9b9b-4973-bf5c-9e1dabfb28e4)

控制逻辑包含用于选择 UART 的各种操作模式的控制寄存器和模式寄存器。控制寄存器对接收机和发送机模块进行启用、禁用和发出软重置。此外，它将重新启动接收器超时时间，并控制发射机中断逻辑。接收断线检测必须在软件中实现。它由一个帧错误和 RXFIFO 中的一个或多个零字节来表示。模式寄存器选择波特率发生器所使用的时钟。它还选择被传输和接收的数据所使用的位长度、奇偶校验位和停止位。此外，它还可以根据需要选择 UART 的操作模式，在正常的 UART 模式、自动回波、本地环回或远程环回之间进行切换。

### UART 工作模式

UART 包含四种工作模式，如图 2-2 所示。

![图2-2 UART的工作模式](https://github.com/ustc-ivclab/.github/assets/32936898/e4e1427c-2793-439c-b1d6-23799db1ed0b)

Mode
Switch 控制器内控制 RxD 和 TxD 信号，其中有四种操作模式，如图 2-2 所示。该模式由寄存器位字段控制。

Normal
Mode 用于标准的 UART 操作。自动回波模式回波模式接收 RxD 上的数据，并且模式开关将数据路由到接收器和 TxD 引脚。来自发射器的数据不能从控制器发送出去。

Automatic Echo
Mode 接收 RxD 上的数据，并且模式开关将数据路由到接收器和 TxD 引脚。来自发射器的数据不能从控制器中发送出来。

Local Loopback
Mode 不连接到 RxD 或 TxD 引脚。相反，传输的数据被环回到接收器。

Remote Loopback
Mode 将 RxD 信号连接到 TxD 信号。在此模式下，控制器不能在 TxD 上发送任何东西，并且控制器在 RxD 上不接收任何东西。

### UART 的 FIFO 中断

FIFO 中断的状态位置如图 2-3 所示。

![图2-3 UART的 FIFO 中断](https://github.com/ustc-ivclab/.github/assets/32936898/d20f9480-6a18-411f-83f5-1ac4489fe87e)

FIFO 触发器级别由以下的寄存器控制字控制：

• uart.Rcvr_FIFO_trigger_level\[RTRIG\]

• uart.Tx_FIFO_trigger_level\[TTRIG\]

### UART 控制器编程流程

UART 控制器编程顺序的流程图如图 2-3 所示。

![图2-3 UART控制流程图](https://github.com/ustc-ivclab/.github/assets/32936898/6964de8e-0303-4897-adb6-b3e9dc1b281f)

## 相关寄存器说明

### UART 控制器寄存器概述

UART 寄存器的概述见表 3-1。

![表3-1 UART寄存器](https://github.com/ustc-ivclab/.github/assets/32936898/9644f786-d16f-4d43-9f51-819449efd352)

### UART 控制器寄存器地址

UART 模块寄存器摘要见表 3-2。

表 3-2 UART 控制器寄存器地址

基地址：0x00FF000000 (UART0) 0x00FF010000 (UART1)

| Register Name                                                                               | Address      | Width | Type  | Reset Value | Description            |
| ------------------------------------------------------------------------------------------- | ------------ | ----- | ----- | ----------- | ---------------------- |
| [Control](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)                 | 0x0000000000 | 32    | mixed | 0x00000128  | UART Control Register  |
| [Mode](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)                    | 0x0000000004 | 32    | mixed | 0x00000000  | UART Mode Register     |
| [Intrpt_en](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)               | 0x0000000008 | 32    | mixed | 0x00000000  | Interrupt Enable       |
| [Intrpt_dis](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)              | 0x000000000C | 32    | mixed | 0x00000000  | Interrupt Disable      |
| [Intrpt_mask](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)             | 0x0000000010 | 32    | mixed | 0x00000000  | Interrupt Mask         |
| [Chnl_int_sts](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)            | 0x0000000014 | 32    | wtc   | 0x00000200  | Channel Interrupt      |
| [Baud_rate_gen](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)           | 0x0000000018 | 32    | mixed | 0x0000028B  | Baud Rate Generator    |
| [Rcvr_timeout](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)            | 0x000000001C | 32    | mixed | 0x00000000  | Receiver Timeout       |
| [Rcvr_FIFO_trigger_level](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html) | 0x0000000020 | 32    | mixed | 0x00000020  | Receiver FIFO Trigger  |
| [Modem_ctrl](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)              | 0x0000000024 | 32    | mixed | 0x00000000  | Modem Control Register |
| [Modem_sts](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)               | 0x0000000028 | 32    | mixed | 0x00000000  | Modem Status Register  |
| [Channel_sts](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)             | 0x000000002C | 32    | ro    | 0x00000000  | Channel Status         |
| [TX_RX_FIFO](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)              | 0x0000000030 | 32    | mixed | 0x00000000  | Transmit and Receive   |
| [Baud_rate_divider](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)       | 0x0000000034 | 32    | mixed | 0x0000000F  | Baud Rate Divider      |
| [Flow_delay](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)              | 0x0000000038 | 32    | mixed | 0x00000000  | Flow Control Delay     |
| [Tx_FIFO_trigger_level](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)   | 0x0000000044 | 32    | mixed | 0x00000020  | Transmitter FIFO       |
| [Rx_FIFO_byte_status](https://www.xilinx.com/htmldocs/registers/ug1087/mod___uart.html)     | 0x0000000048 | 32    | mixed | 0x00000000  | RX FIFO Byte Status    |

## 中心机与搭载板的工作状态

### 中心机与搭载板之间的通信和数据传输

中心机与搭载板之间通信方式和数据传输过程如图 4-1 所示。

![图4-1 中心机与搭载板的数据传输图](https://github.com/ustc-ivclab/.github/assets/32936898/3a0bc161-5c94-4c0f-9a34-481eb99dfdae)

- 中心机启动搭载板之后，搭载板完成开机初始化，与中心机通过 RS-422 串口建立通信连接。
- 建立连接之后，中心机可通过查询搭载板命令查询搭载板状态。查询搭载板命令和搭载板返回复命令如下：
- 中心机查询搭载板命令：00 01 00 00 00 00 00 00 00 00 97 7D
- 搭载板回复命令：
- 处于接收图片状态：01 01 00 00 00 00 00 00 00 01 92 ED
- 处理图片完成状态：01 01 00 00 00 00 00 00 00 10 50 F7
- 处于处理图片状态：01 01 00 00 00 00 00 00 00 00 88 88
- 当搭载板处于接收图片状态，中心机可以查询搭载板状态和发送，搭载板返回允许指令后，中心机即可向搭载板发送图片数据，发送完成后返回接收完成命令。在数据传输过程中搭载板如果发现传输的某帧数据有误，可以向中心机发送重发错误帧数据的命令，中心机收到命令后，会重新发送该帧数据。如果接收超时或者接收的整个数据有误，返回接收失败命令。当搭载板处于其他状态时中心机向搭载板发送发送图片数据搭载板将不会接收。
- 当搭载板接收完图片数据后，将会自动进入处理图片状态。此时将不再接收中心机的相关命令。
- 处理完图片数据后，搭载板自动进入图片处理完成状态，中心机可以查询搭载板状态并向搭载板发送请求数据命令，搭载板收到命令后将向中心机发送相关数据（搭载板向中心机发数据时，每过 m 帧以后，间隔 t 毫秒，以便给中心机留出一点处理时间。m 和 t 作为待定参数，由中心机稍后确定）

### 中心机与搭载板之间的相关设置

中心机与搭载板之间的硬件连接关系如图 4-2 所示。

![图4-2 中心机与搭载板的硬件连接图](https://github.com/ustc-ivclab/.github/assets/32936898/f48103d3-09d3-48de-b3fe-9d21acd108cb)

![图4-3 注数时序图](https://github.com/ustc-ivclab/.github/assets/32936898/27b074f5-25f9-4a27-b696-9534b08594fe)

1. 配置通信参数：两个装置之间需要设置相同的通信参数。波特率为 921600；通信方式采用标准串行协议；通信的基本单位为字符，每个字符传输时为 11 位（1 位起始+8 位数据+1 位奇校验+1 位停止），其中数据位传输字节内先传低位（D0），最后传高位（D7），如图 4-3 所示。
2. 连接硬件：RS-422 通信需要使用两对差分信号线，分别命名为 A+、A-和 B+、B-。这些信号线需要通过连接线缆正确地连接到两个装置上。
3. RS-422 发送数据和接收数据
4. 中心机发送数据：配置完通信参数并建立连接之后，中心机将数据转换为差分信号，并通过 A+、A-信号线发送，同时监听 B+、B-信号线上的差分信号。
5. 搭载板接收数据：搭载板监听 A+、A-信号线上的差分信号，并将其转换为相应的数据，同时通过 B+、B-信号线发送差分信号。
6. 搭载板发送数据：配置完通信参数并建立连接之后，搭载板将数据转换为差分信号，并通过 B+、B-信号线发送，同时监听 A+、A-信号线上的差分信号。
7. 搭载板接收数据：搭载板监听 B+、B-信号线上的差分信号，并将其转换为相应的数据，同时通过 A+、A-信号线发送差分信号。
8. 确认和错误检测：发送方装置在发送命令或数据后，会等待接收方装置的返回信号。如果接收方装置成功接收到数据，它将发送一个返回信号。如果发送方装置未收到返回信号或者接收到数据出错的信号，发送方可以重新发送数据。

## 数据传输格式

数据传输采用定长帧，具体的数据流格式见表 5-1 和表 5-2。

表 5-1 数据流状态

| 帧序号 | 1    | 1      | 1      | 1          | 1        | 1              | 1                      | 1        | 1   | 2    | 2      | 2      | 2          | 2        | 2              | 2                      | 2        | 2   | …   | 3    | 3      | 3      | 3          | 3        | 3              | 3                      | 3        | 3   |
| ------ | ---- | ------ | ------ | ---------- | -------- | -------------- | ---------------------- | -------- | --- | ---- | ------ | ------ | ---------- | -------- | -------------- | ---------------------- | -------- | --- | --- | ---- | ------ | ------ | ---------- | -------- | -------------- | ---------------------- | -------- | --- |
| 名称   | 帧头 | 帧计数 | 标志字 | 数传文件号 | 帧内包号 | 压缩数据总长度 | 当前包有效压缩数据长度 | 压缩数据 | CRC | 帧头 | 帧计数 | 标志字 | 数传文件号 | 帧内包号 | 压缩数据总长度 | 当前包有效压缩数据长度 | 压缩数据 | CRC | …   | 帧头 | 帧计数 | 标志字 | 数传文件号 | 帧内包号 | 压缩数据总长度 | 当前包有效压缩数据长度 | 压缩数据 | CRC |
| 字节   | 4    | 3      | 1      | 4          | 2        | 4              | 2                      | 512      | 2   | 4    | 3      | 1      | 4          | 2        | 4              | 2                      | 512      | 2   | …   | 4    | 3      | 1      | 4          | 2        | 4              | 2                      | 512      | 2   |

表 5-2 数据格式(一帧)

| 字节序号 | 名称                   | 字节 | 定义或处理方法                                                                                                                                                                     | 备注                                                    |
| -------- | ---------------------- | ---- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------- |
| 1~4      | 帧头                   | 4    | 0x3A62043F                                                                                                                                                                         | -                                                       |
| 5~7      | 帧计数                 | 3    | 从 0 往上计数                                                                                                                                                                      | 除开机外，计数器不清零                                  |
| 8        | 标志字                 | 1    | 相机 1 摄像模式：0x8A(H.265)                                                                                                                                                       | -                                                       |
| 同上     | 标志字                 | 同上 | 相机 1 照相模式：0x8C(JPEG)                                                                                                                                                        | -                                                       |
| 同上     | 标志字                 | 同上 | 相机 1 照相模式：0x8E(YUV420)                                                                                                                                                      | -                                                       |
| 同上     | 标志字                 | 同上 | 相机 2 摄像模式：0x9A(H.265)                                                                                                                                                       | -                                                       |
| 同上     | 标志字                 | 同上 | 相机 2 照相模式：0x9C(JPEG)                                                                                                                                                        | -                                                       |
| 同上     | 标志字                 | 同上 | 相机 2 照相模式：0x9E(YUV420)                                                                                                                                                      | -                                                       |
| 同上     | 标志字                 | 同上 | 搭载板回传数据：0xAA                                                                                                                                                               | -                                                       |
| 9~12     | 数传文件号             | 4    | -                                                                                                                                                                                  | -                                                       |
| 13~14    | 帧内包号               | 2    | （1）有且只有 1 包：0x0000；                                                                                                                                                       | -                                                       |
| 同上     | 帧内包号               | 同上 | （2）多于 1 包的首包：0x0001；                                                                                                                                                     | -                                                       |
| 同上     | 帧内包号               | 同上 | 其他顺序编号。                                                                                                                                                                     | -                                                       |
| 15~18    | 压缩数据总长度         | 4    | 压缩数据总长度（压缩数据总长度，也称为与数传文件号对应的数传文件长度，数传文件长度是同一文件号下所有包有效压缩数据长度的总和，同一文件号对应的存储文件长度与数传文件长度是一致的） | 压缩数据长度 n1 以 16 进制表示                          |
| 19~20    | 当前包有效压缩数据长度 | 2    |                                                                                                                                                                                    | n2 以 16 进制表示                                       |
| 21~532   | 压缩数据               | 512  |                                                                                                                                                                                    | 前 n2 个字节为有效压缩数据，有效数据不足 512 字节的补零 |
| 533~534  | CRC 校验               | 2    | 生成多项式为： $G(X) = X^16+X^12+X^5+1$ 寄存器初相设为全 1                                                                                                                         | CRC-16                                                  |

说明：

- 表中一字节为 8bit；
- 数据输出时，每个域的高字节在前，低字节在后；字节内高位 B7 在前，低位 B0 在后。例如：帧计数 3 个字节，先输出最高字节，最后输出最低字节。帧头 4 字节，依次输出为：0x3A、0x62、0x04、0x3F

## RS422 通信协议

### 通信格式

搭载版以串行异步通信接口接收中心机发送的控制指令、数据输入，向中心机返回应答指令，发送数据。接口电平为 RS422，数据速率：92.16KB/s。

通信方式采用标准串行协议；通信的基本单位为字符，每个字符传输时为 11 位（1 位起始+8 位数据+1 位奇校验+1 位停止），其中数据位传输字节内先传低位（D0），最后传高位（D7），如图 6-1 所示。

![图6-1注数时序图](http://mathdesc.fr/documents/serial/serial_fichiers/async.gif)

多字节数据传送：先传送最高字节数据，然后传送次高字节数据，最后传送最低字节数据。

### 指令类型

中心机与搭载板之间通信及传输数据的指令类型如下表所示。

表 6-1 查询命令格式

| 序号 | 字节定义          | 描述                     | 备注   |
| ---- | ----------------- | ------------------------ | ------ |
| 1    | 主机 00H/从机 01H | 主机/从机号              | 1 字节 |
| 2    | 查询 01H          | 指令类型                 | 1 字节 |
| 3    | 0xXXXXXXXX        | 数据文件编号             | 4 字节 |
| 4    | 0xXXXX            | 完成状态返回数据文件帧数 | 2 字节 |
| 同上 | 0xXXXX            | 接收状态返回成功接收帧数 | 同上   |
| 同上 | 0xXXXX            | 处理状态则表示已成功接收 | 同上   |
| 5    | 接收状态 0xAAAA   | 搭载板工作状态           | 2 字节 |
| 同上 | 处理状态 0xAA55   | 搭载板工作状态           | 同上   |
| 同上 | 完成状态 0x5555   | 搭载板工作状态           | 同上   |
| 6    | 0xXXXX            | CRC 校验                 | 2 字节 |

表 6-2 传输数据请求命令格式

| 序号 | 字节定义          | 描述         | 备注   |
| ---- | ----------------- | ------------ | ------ |
| 1    | 主机 00H/从机 01H | 主机/从机号  | 1 字节 |
| 2    | 数据请求 03H      | 指令类型     | 1 字节 |
| 3    | 0x00000000        | 数据文件编号 | 4 字节 |
| 4    | 0x0000XXXX        | 数据文件帧数 | 4 字节 |
| 5    | 0x0000            | CRC 校验     | 2 字节 |

表 6-4 中心机要数据命令格式

| 序号 | 字节定义   | 描述         | 备注   |
| ---- | ---------- | ------------ | ------ |
| 1    | 主机 00H   | 主机号       | 1 字节 |
| 2    | 要数据 07H | 指令类型     | 1 字节 |
| 3    | 0x00000000 | 数据文件编号 | 4 字节 |
| 4    | 0x00000000 | 帧序号       | 4 字节 |
| 5    | 0x0000     | CRC 校验     | 2 字节 |

### 指令

### 中心机查询搭载板命令：

- 00 01 00 00 00 00 00 00 00 00 97 7D

### 搭载板回复命令：

- 处于接收图片状态： 01 01 00 00 00 00 00 00 00 01 92 ED
- 处于接收图片状态： 01 01 XX XX XX XX 00 00 00 01 92 ED
- 处于处理图片状态： 01 01 00 00 00 00 00 00 00 00 52 2C
- 处理图片完成状态（红色部分为返回二进制码流的帧数）：01 01 XX XX XX XX 00 00 00 10 50 F7

### 中心机传输数据请求命令：

00 03 00 00 00 01 00 00 7E 90 FB 78

### 搭载版返回传输数据请求命令：

- 01 03 00 00 00 00 00 00 00 00 F2 08（允许）
- 01 03 00 00 00 00 00 00 00 01 32 C9（拒绝）

### 中心机要数据命令：

- 00 07 00 00 00 01 00 00 00 00 F7 6B

## RS-422 硬件接口

RS-422 接口原理图如图 7-1 所示。

![图7-1 RS-422 接口原理图](https://github.com/ustc-ivclab/.github/assets/32936898/59f7bc6b-1107-4dd5-b2a3-6514ba2f32b4)

## 参考文献

- [Zynq UltraScale+ Device Technical Reference Manual](https://www.xilinx.com/content/dam/xilinx/support/documents/user_guides/ug1085-zynq-ultrascale-trm.pdf)
- [Zynq UltraScale+ MPSoC Register Reference](https://docs.xilinx.com/r/en-US/ug1087-zynq-ultrascale-registers/UART-Module)

<!-- ex: nowrap
-->
