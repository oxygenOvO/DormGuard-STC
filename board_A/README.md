# A 板：现场感知节点

负责人：成员 1。

计划范围包括 Hall 门磁、Vib 振动、LED/蜂鸣器、布防与报警状态机、A↔B 串口通信、A 端协议和心跳发送。

`firmware/` 已导入课程提供的霍尔验证工程；后续功能仍须以真实 BSP 声明和已验证示例为依据，不创建或猜测任何 BSP 接口。

## Hall 门状态检测

A 板固件以课程资料中已经运行验证的“霍尔”Keil C51 工程为基础，保持其工程文件、启动文件、BSP 库和 `../inc` 头文件相对路径。

使用的真实接口：

- `HallInit()`：初始化 Hall 模块；
- `GetHallAct()`：读取一次性 Hall 变化事件；
- `enumEventHall`：Hall 事件类型，通过 `SetEventCallBack()` 注册回调；
- `enumHallGetClose`：磁场接近；
- `enumHallGetAway`：磁场离开。

DormGuard 当前映射：

```text
enumHallGetClose -> DOOR_STATE_CLOSED
enumHallGetAway  -> DOOR_STATE_OPEN
```

该映射假定门关闭时磁铁靠近 Hall，仍需根据实际门磁安装位置进行实机确认。

上电初始状态为 `DOOR_STATE_UNKNOWN`。BSP 文档没有保证 `HallInit()` 会为当前静态电平主动产生事件，因此只有首次收到 `enumHallGetClose` 或 `enumHallGetAway` 后，门状态才更新为 `CLOSED` 或 `OPEN`。

LED 调试反馈使用真实接口 `LedPrint()`：UNKNOWN 为 `0x00`（全灭），CLOSED 为 `0x01`（LED1），OPEN 为 `0x02`（LED2）。LED 位序也应随门磁映射一起完成一次实机确认。

Hall 阶段实现了门状态采集和 LED 调试；后续阶段在此基础上继续加入 Vib 与安全状态机，尚不包含 UART 或心跳。

### Hall 软件逻辑状态

- Hall 软件处理层：已实现；
- Hall 软件模拟测试：已实现，等待在 Keil 中编译和调试观察；
- Hall 真实硬件验证：待磁铁到位后完成。

`process_hall_action()` 独立负责把 BSP Hall 事件转换成门状态。真实 `hall_callback()` 只调用一次 `GetHallAct()`，再把读取结果交给该函数。`enumHallNull` 不改变状态；只有新状态与当前状态不同时，才将 `door_changed` 置为 `1`。

`door_changed` 表示存在尚未处理的门状态变化。当前安全状态机处理完变化后将其清零；独立 Hall 软件测试会在每个测试步骤之间显式清零。

### 软件模拟测试

将 `HALL_SOFT_TEST` 设置为 `1` 可运行独立 Hall 测试。该模式直接向 `process_hall_action()` 依次输入 Close、重复 Close、Away、Null、Close，不注册真实 Hall 回调，也不修改 BSP。当前状态机测试模式下该宏为 `0`。

在 Keil 调试器中观察：

- `hall_soft_test_completed == 1`：测试流程已经执行；
- `hall_soft_test_failures == 0`：五项软件状态测试全部通过；
- 非零结果的 bit0～bit4 分别表示 TEST 1～TEST 5 失败。

需要连接真实 Hall 硬件时，将：

```c
#define HALL_SOFT_TEST 1
```

改为：

```c
#define HALL_SOFT_TEST 0
```

关闭后工程会重新执行 `HallInit()`，并注册 `enumEventHall` 对应的真实 `hall_callback()`。

尚未经过磁铁实机验证的项目：

1. `HallInit()` 后是否会为当前静态状态产生初始事件；
2. 实际安装时 `enumHallGetClose` 是否对应门关闭；
3. 实际安装时 `enumHallGetAway` 是否对应门打开。

软件模拟测试只能验证 DormGuard 状态转换逻辑，不能替代最终磁铁实机测试。

## Vib 振动检测

使用课程 BSP 中已经确认的真实接口：

- `VibInit()`：初始化 Vib 模块；
- `GetVibAct()`：读取一次性 Vib 事件，读取后事件值恢复为 `enumVibNull`；
- `enumEventVib`：Vib 事件类型，通过 `SetEventCallBack()` 注册回调；
- `enumVibNull`：没有新的振动事件；
- `enumVibQuake`：检测到一次有效振动事件。

真实 `vib_callback()` 只调用一次 `GetVibAct()`，然后将结果交给 `process_vib_action()`。处理层收到 `enumVibQuake` 后设置 `vib_event = 1` 并递增 `vib_event_count`；收到 `enumVibNull` 时不修改事件和计数。本阶段不会触发蜂鸣器或报警状态。

`vib_event` 表示一个尚未消费的振动软件事件。`clear_vib_event()` 用于在测试或未来状态机处理完成后清除该标志。`vib_event_count` 用于累计 BSP 报告的有效振动次数。

将 `VIB_SOFT_TEST` 设置为 `1` 可运行三项独立软件测试：首次 Quake、消费后再次 Quake、Null 不误触发。当前状态机测试模式下该宏为 `0`。在 Keil 调试器中观察：

- `vib_soft_test_completed == 1`：测试流程已经执行；
- `vib_soft_test_failures == 0`：三项测试全部通过；
- 非零结果的 bit0～bit2 分别表示 TEST V1～TEST V3 失败。

关闭软件测试时，将 `VIB_SOFT_TEST` 改为 `0`。工程将执行 `VibInit()` 并注册真实 `enumEventVib` 回调。真实 Vib 硬件触发灵敏度和事件表现仍需实机验证。

## A 板感知层状态

- Hall：门状态检测软件层已实现，磁铁物理测试待完成；
- Vib：振动事件软件层已实现，软件测试等待 Keil 验证，真实硬件测试待完成；
- 状态机：尚未实现；
- UART：尚未实现；
- Heartbeat：尚未实现。

## LED 位分配

100 ms 周期回调统一构造一个 `led_value`，并且只调用一次 `LedPrint()`：

- bit0（`0x01`）：门关闭；
- bit1（`0x02`）：门打开；
- bit2（`0x04`）：Vib 报警来源；独立 Vib 软件测试模式下也表示测试已完成且无失败；
- 门状态 UNKNOWN 且没有 Vib 指示时为 `0x00`。

Hall 和 Vib 回调均不直接更新 LED，因此不会互相覆盖显示。

## Security State Machine

A 板安全状态机包含：

- `SECURITY_STATE_DISARMED`：默认撤防状态，持续更新传感器状态，但消费并丢弃 pending 门变化和 Vib 事件；
- `SECURITY_STATE_ARMED`：布防状态，等待门打开或 Vib 事件；
- `SECURITY_STATE_ALARM`：锁存报警状态，只能通过撤防或满足条件的报警复位退出。

业务控制函数与未来 UART 传输层解耦：

- `security_arm()`：只有 `door_state == DOOR_STATE_CLOSED` 才允许布防；UNKNOWN 和 OPEN 均拒绝；
- `security_disarm()`：从任意状态进入 DISARMED，清除报警原因和 pending 事件，但保留真实门状态；
- `security_reset_alarm()`：只有处于 ALARM 且门已 CLOSED 时才清除报警并重新进入 ARMED；门为 OPEN 或 UNKNOWN 时拒绝。

报警原因采用位标志，可同时保存多个来源：

```text
ALARM_FLAG_NONE = 0x00
ALARM_FLAG_DOOR = 0x01
ALARM_FLAG_VIB  = 0x02
```

`process_security_state()` 由 100 ms 回调调用。ARMED 状态下，门变化为 OPEN 时锁存 DOOR 原因，Vib 事件则锁存 VIB 原因；ALARM 期间的新事件继续补充 `alarm_flags`。DISARMED 状态只消费 pending 事件，不报警。

### 蜂鸣器报警执行层

蜂鸣器使用课程真实接口：`BeepInit()`、`GetBeepStatus()` 和 `SetBeep()`。第一次从非 ALARM 状态进入 ALARM 时设置一次 `alarm_beep_pending`；100 ms 执行层确认蜂鸣器空闲后调用：

```c
SetBeep(1200, 100);
```

该组参数来自课程 Vib 示例，表示 1200 Hz、约 1000 ms 的非阻塞提示。调用成功后清除 pending 标志，因此不会每 100 ms 重启蜂鸣器。撤防或成功 RESET 会取消尚未执行的提示；BSP 没有提供已确认的“立即停止当前声音”接口，因此已经开始的提示可能继续到本次时长结束。

### 状态机软件测试

当前测试宏配置：

```c
#define HALL_SOFT_TEST  0
#define VIB_SOFT_TEST   0
#define STATE_SOFT_TEST 0
#define UART_SOFT_TEST  0
```

当前为真实硬件联调配置。软件测试代码仍保留；需要单独运行某项软件测试时，只启用对应宏，并保证其余测试宏为 `0`。

状态机测试直接调用 Hall/Vib 软件处理函数和安全控制函数，不伪造 BSP。十项测试覆盖：默认撤防、UNKNOWN/OPEN 拒绝布防、CLOSED 成功布防、门报警、撤防、Vib 报警、关门 RESET、开门拒绝 RESET，以及撤防期间旧 Vib 不得污染后续布防。

在 Keil 调试器中观察：

- `state_soft_test_completed == 1`：测试已经执行；
- `state_soft_test_failures == 0`：十项测试全部通过；
- 非零结果的 bit0～bit9 分别表示 TEST S1～TEST S10 失败。

独立状态机测试时应将 `STATE_SOFT_TEST` 设置为 `1`、其他测试宏设置为 `0`。正式硬件测试前必须将四个测试宏全部设置为 `0`，从而恢复真实 Hall/Vib/UART 初始化和事件回调。

### 完成状态

- 状态机软件逻辑：已实现，待 Keil C51 编译和调试执行；
- 蜂鸣器报警执行层：已实现，待 Keil C51 和实机验证；
- Hall 物理测试：仍待磁铁；
- Vib 物理测试：仍待实机确认；
- UART：协议接入和软件测试已实现，物理层已迁移到 UART2/EXT，双板联调待完成；
- Heartbeat：已实现，实机接收与掉线联调待完成。

统一 LED 位分配现为：

```text
bit0 / 0x01：门关闭
bit1 / 0x02：门打开
bit2 / 0x04：Vib 报警来源
bit3 / 0x08：ARMED
bit4 / 0x10：ALARM
```

## UART Communication

A 板通过 UART2 的 EXT TTL 接口与 B 板通信，PC 不直接连接 A 板。B 板总体上应使用 UART2/EXT 与 A 板连接，并将 UART1/USB 保留给 PC 上位机。当前沿用课程双机 UART2 示例中已经使用的 `1200 bps`；通信格式由 BSP 固定为 8 数据位、1 停止位、无奇偶校验。A、B 两板必须使用完全相同的串口配置。

课程 BSP 文档确认：UART1 固定连接学习板 USB 接口；UART2 可初始化到 EXT 扩展插座（TTL 全双工）或 485 接口（半双工）。DormGuard 板间直连选择 `Uart2UsedforEXT`，不使用 485 模式，也不调用 `EXTInit()`，以避免 EXT 功能冲突。

真实 BSP 接口：

- `Uart2Init(1200, Uart2UsedforEXT)`：将 UART2 初始化到 EXT TTL 接口；
- `SetUart2Rxd(&uart_rx_byte, 1, 0, 0)`：配置无帧头匹配的单字节接收；
- `enumEventUart2Rxd`：收到一个字节后的回调事件；
- `Uart2Print(&uart_tx_byte, 1)`：非阻塞发送一个字节；
- `GetUart2TxStatus()`：发送前确认 UART2 空闲。

板间 TTL 基本连接：

```text
A 板 EXT UART2 TX -> B 板 EXT UART2 RX
A 板 EXT UART2 RX <- B 板 EXT UART2 TX
A 板 GND          <-> B 板 GND
```

两板 TTL 电平兼容性、EXT 插座上 TX/RX/GND 的具体针脚顺序，以及是否需要跳帽或拨码设置，当前课程文字资料没有给出足够信息，必须按板卡丝印/原理图实机确认。不要把 485 的 A/B 差分端子按上述 TTL 方式交叉连接。

UART 接收回调只读取 BSP 已写入的全局 `uart_rx_byte`，再调用 `process_uart_command()`。命令处理与安全状态机解耦：

- `CMD_ARM (0xA1)`：调用 `security_arm()`；成功发送 `MSG_ARM_OK`，门为 OPEN 时失败并发送 `MSG_DOOR_OPEN`；
- `CMD_DISARM (0xA2)`：调用 `security_disarm()`，然后发送 `MSG_DISARM_OK`；
- `CMD_RESET (0xA3)`：调用 `security_reset_alarm()`，当前协议没有 RESET 响应；
- 其他字节：忽略。

发送层使用 8 字节小型非阻塞队列。`Uart2Print()` 与 UART1 版本一样是非阻塞调用，因此队列中的全局 `uart_tx_byte` 在 BSP 完成异步发送前保持有效；队列满时递增 `uart_tx_drop_count`。每个100 ms周期最多启动一个新字节发送，迁移后队列和上层协议逻辑不变。

### 主动上报与去重

Hall 状态真正变化时同时设置 `door_changed` 和独立的 `door_report_pending`。状态机可以消费前者，UART仍通过后者看到同一次变化：

- CLOSED：发送一次 `MSG_DOOR_CLOSE (0xB4)`；
- OPEN：发送一次 `MSG_DOOR_OPEN (0xB3)`。

`alarm_reported_flags` 独立记录已经排队上报的报警原因：

- Door 原因首次出现：发送一次 `MSG_DOOR_ALARM (0xB5)`；
- Vib 原因首次出现：发送一次 `MSG_VIB_ALARM (0xB6)`；
- 同一锁存报警期间不重复发送；
- 后续出现另一种原因时仍会单独发送；
- ARM、DISARM 或成功 RESET 会清除去重标志。

### Heartbeat

使用真实 `enumEventSys1S` 周期事件，每秒将 `MSG_HEARTBEAT (0xC1)` 加入发送队列，不使用阻塞 delay。A 板只负责发送；超时和在线判断由 B 板负责。

### UART 软件测试

UART 软件测试代码仍保留。将 `UART_SOFT_TEST` 临时设置为 `1` 后，`send_protocol_message()` 只记录 `last_tx_message`、`tx_message_count` 和各消息计数，不调用真实 UART BSP。七项测试覆盖：ARM成功、OPEN拒绝ARM、DISARM、Door报警一次上报、Vib报警一次上报、未知命令忽略，以及门 OPEN/CLOSE 变化各上报一次。当前联调版本设置为 `UART_SOFT_TEST = 0`，使用真实 UART2 BSP。

Keil 调试器预期：

```text
uart_soft_test_completed == 1
uart_soft_test_failures  == 0
```

正式双板测试前将：

```text
HALL_SOFT_TEST  = 0
VIB_SOFT_TEST   = 0
STATE_SOFT_TEST = 0
UART_SOFT_TEST  = 0
```

### 当前协议缺口

以下情况暂不新增消息值，等待三人共同确认协议：

1. ARM + UNKNOWN 没有专用失败响应；
2. ALARM 状态下 ARM 失败没有专用响应；
3. CMD_RESET 没有 RESET_OK；
4. CMD_RESET 没有 RESET_FAILED；
5. 第一版单字节协议没有校验、序号或 ACK 机制。

UART软件逻辑和心跳调度已经实现，A 板物理层已迁移到 UART2/EXT；尚未完成 A↔B 实机串口联调。课程参考工程在 UART2/485 模式下验证了 1200 bps，BSP 接口本身允许为 EXT 模式设置相同波特率；EXT 模式下的 1200 bps 仍需两板实测确认。

## A/B Hardware Link Test

联调参数：UART2、EXT TTL、1200 bps、8 数据位、1 停止位、无奇偶校验。

`SetUart2Rxd(&uart_rx_byte, 1, 0, 0)` 只在初始化阶段设置一次。BSP V3.6b 说明指出，UART2 接收事件的用户回调返回后，系统才接收下一个数据包；课程 UART2 接收示例同样没有在回调中重新调用 `SetUart2Rxd()`。因此当前回调不重复挂载接收缓冲区。

Keil Watch 诊断变量：

- `uart_rx_count`：真实 UART2 接收回调次数；
- `uart_tx_success_count`：`Uart2Print()` 成功接受发送请求的次数；
- `uart_tx_drop_count`：发送队列已满导致消息无法入队的次数；
- `uart_last_rx_byte`：最近一次真实接收的字节；
- `uart_last_tx_byte`：最近一次成功启动发送的字节。

最小联调链路：

```text
A -> B: 0xC1  MSG_HEARTBEAT，每秒一次
B -> A: 0xA2  CMD_DISARM
A -> B: 0xB2  MSG_DISARM_OK
```

先让 B 板连续接收 A 板心跳至少 10 秒，确认 A 板的 `uart_tx_success_count` 持续增加、`uart_last_tx_byte == 0xC1` 且 `uart_tx_drop_count == 0`。随后由 B 板发送 `0xA2` 至少 10 次；每次应使 `uart_rx_count` 增加、`uart_last_rx_byte == 0xA2`，并让 A 板恰好返回一个 `0xB2`。

没有磁铁不影响上述 UART 基础联调。第一次测试不要使用 `CMD_ARM (0xA1)`：上电时 `door_state` 很可能为 UNKNOWN，按安全规则拒绝布防是正确行为，不能据此判断串口故障。

8 字节 TX 队列在当前正常负载下是合理的：基础负载只有每秒 1 字节心跳，100 ms 服务周期理论上最多启动每秒 10 个单字节发送。门状态、报警和命令响应可能短时叠加，因此联调期间必须持续观察 `uart_tx_drop_count`；若其保持为 `0`，本阶段不调整队列。
