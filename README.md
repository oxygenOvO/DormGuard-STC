# DormGuard 宿舍安全联防系统

DormGuard 是一个基于两块 STC15F2K60S2 开发板和 PC Qt 上位机的三级安全监测系统。A 板负责现场感知与报警，B 板负责控制、状态汇总和通信转发，PC 上位机负责集中监控、远程控制与事件记录。

## 系统架构

```text
Hall / Vib
    ↓
A板：现场感知与报警
    ↕ UART
B板：控制与通信网关
    ↕ Serial
PC：Qt上位机
```

系统主要解决：

1. 离开宿舍后无法确认门状态；
2. 宿舍无人时异常开门无法及时发现；
3. 门体受到明显撞击时无法及时报警；
4. 需要在 PC 端集中查看系统状态和报警记录。

## 三人分工

- 成员 1（`board_A/`）：负责 Hall 门磁、Vib 振动、LED/蜂鸣器、布防与报警状态机、A↔B 串口收发、A 端协议和心跳发送。
- 成员 2（`board_B/`）：负责 K1/K2/K3、数码管、LED/蜂鸣器、状态管理、A 板消息处理、PC 指令处理、双向转发、心跳超时和掉线判断。
- 成员 3（`pc_host/`）：负责 Qt/C++、QSerialPort、串口连接、实时状态、布防/撤防/报警确认、报警提示、事件日志及后期 CSV 导出。
- `docs/` 由三人共同维护。

## 开发原则

- 优先使用课程已有 BSP；
- 不自行编写不必要的底层驱动；
- 采用事件驱动方式；
- 避免长时间阻塞式 delay；
- 三端统一遵守 `docs/protocol.md`；
- 修改通信协议必须同步修改协议文档；
- 每完成一个独立功能就进行实机测试和 Git commit。

## Git 协作

- `main`：仅保存稳定、可演示版本。
- 成员 1 使用 `feature/board-a`。
- 成员 2 使用 `feature/board-b`。
- 成员 3 使用 `feature/pc-host`。

建议提交信息：

```text
feat(board-a): add hall door detection
feat(board-a): add vibration detection
feat(board-b): add local control
feat(board-b): add uart gateway
feat(pc): add serial connection
feat(pc): add alarm display
docs(protocol): update message definition
fix(board-a): prevent duplicated alarm event
```

禁止使用 `update`、`final`、`test`、`最新版` 等无法说明改动内容的提交信息。

## 当前阶段

仓库目前只包含项目骨架和设计文档。课程提供的真实 BSP 与已验证的 STC15 工程应在后续确认接口后导入；当前不包含 Hall、Vib、UART、状态机或 Qt 界面的业务实现。
