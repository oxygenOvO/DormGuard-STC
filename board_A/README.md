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
