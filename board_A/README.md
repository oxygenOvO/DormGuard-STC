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

当前阶段只实现 Hall 门状态采集和 LED 调试，不包含 Vib、蜂鸣器报警、完整状态机、UART 或心跳。

### Hall 软件逻辑状态

- Hall 软件处理层：已实现；
- Hall 软件模拟测试：已实现，等待在 Keil 中编译和调试观察；
- Hall 真实硬件验证：待磁铁到位后完成。

`process_hall_action()` 独立负责把 BSP Hall 事件转换成门状态。真实 `hall_callback()` 只调用一次 `GetHallAct()`，再把读取结果交给该函数。`enumHallNull` 不改变状态；只有新状态与当前状态不同时，才将 `door_changed` 置为 `1`。

`door_changed` 表示存在尚未处理的门状态变化。未来状态机处理完变化后必须将其清零；当前软件测试会在每个测试步骤之间显式清零。

### 软件模拟测试

`霍尔.c` 中的 `HALL_SOFT_TEST` 默认为 `1`。测试模式直接向 `process_hall_action()` 依次输入 Close、重复 Close、Away、Null、Close，不注册真实 Hall 回调，也不修改 BSP。

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
