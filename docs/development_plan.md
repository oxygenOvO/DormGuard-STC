# 并行开发计划

## 成员 1：A 板

1. 阶段 1：Hall
2. 阶段 2：Vib
3. 阶段 3：LED / Beep
4. 阶段 4：DISARM / ARMED / ALARM 状态机
5. 阶段 5：A 端 UART
6. 阶段 6：Heartbeat
7. 阶段 7：A ↔ B 联调

## 成员 2：B 板

1. 阶段 1：Key
2. 阶段 2：Seg7 / LED / Beep
3. 阶段 3：B 板状态变量
4. 阶段 4：与 A 板通信
5. 阶段 5：Heartbeat timeout
6. 阶段 6：与 PC 通信
7. 阶段 7：Gateway 数据转发

## 成员 3：PC 上位机

1. 阶段 1：Qt 项目
2. 阶段 2：QSerialPort
3. 阶段 3：串口打开 / 关闭
4. 阶段 4：状态显示
5. 阶段 5：ARM / DISARM / RESET
6. 阶段 6：报警显示
7. 阶段 7：事件日志
8. 阶段 8：CSV 导出（加分功能）

## 联调顺序

1. A 板单板测试
2. B 板单板测试
3. PC 单独串口测试
4. A ↔ B
5. B ↔ PC
6. PC → B → A
7. A → B → PC
8. 完整报警闭环

## Git 协作规则

- `main`：稳定、可演示版本。
- 成员 1：`feature/board-a`。
- 成员 2：`feature/board-b`。
- 成员 3：`feature/pc-host`。
- 功能分支完成独立功能、通过对应测试后再合入 `main`。

建议使用可读的提交信息，例如：

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

禁止使用 `update`、`final`、`test`、`最新版` 等没有说明实际改动的提交信息。每个提交应小而独立，并在提交前完成实机验证。
