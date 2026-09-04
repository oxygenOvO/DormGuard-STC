# 系统架构

DormGuard 采用 A 板、B 板和 PC 上位机三级结构。A 板完成现场感知，B 板完成本地控制与网关转发，PC 完成人机交互和集中记录。

## A 板：现场感知节点（成员 1）

- Hall：门磁状态采集
- Vib：振动事件采集
- LED：状态指示
- Beep：本地声音报警
- 状态机：维护 DISARM、ARMED、ALARM
- UART：与 B 板交换命令和事件
- Heartbeat：周期报告在线状态

## B 板：控制与通信网关（成员 2）

- Key：K1/K2/K3 本地操作
- Seg7：数码管状态显示
- LED：状态指示
- Beep：本地提示与报警
- 状态管理：保存布防、门、报警与连接状态
- UART：与 A 板通信
- PC 串口通信：接收 PC 控制并上报状态
- Gateway：在 A 板与 PC 之间转发数据

## PC：Qt 上位机（成员 3）

- QSerialPort：连接 B 板
- 状态监控：显示布防、门、报警和在线状态
- 远程控制：发送 ARM、DISARM、RESET
- 报警：显示报警提示
- 日志：记录重要事件，后期可扩展 CSV 导出

`docs/` 由三人共同维护。

## 数据流

```text
PC → B → A：ARM / DISARM / RESET
A → B → PC：门状态 / 报警状态 / 振动报警 / 在线状态
```

B 板是通信网关：向 A 板转发 PC 控制命令，并向 PC 转发 A 板事件及 B 板维护的状态。
