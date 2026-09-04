# 状态机设计

本文档只描述状态设计，不包含业务代码或底层驱动实现。

## A 板状态

### DISARM

- 系统处于撤防状态。
- 正常开门或检测到振动时，不触发入侵报警。
- 收到 `CMD_ARM` 时：如果门已经关闭，则进入 `ARMED` 并报告 `MSG_ARM_OK`；否则保持 `DISARM` 并报告 `MSG_DOOR_OPEN`。

### ARMED

- 系统处于布防状态。
- 门异常打开时进入 `ALARM`，报告 `MSG_DOOR_ALARM`。
- 检测到有效异常振动时进入 `ALARM`，报告 `MSG_VIB_ALARM`。
- 收到 `CMD_DISARM` 时返回 `DISARM`，并报告 `MSG_DISARM_OK`。

### ALARM

- 系统保存报警状态并进行声光提示。
- 收到 `CMD_DISARM` 时返回 `DISARM`。
- 收到 `CMD_RESET` 时，按照后续确定的报警恢复策略执行；当前版本不提前规定其目标状态。

## B 板记录状态

B 板维护并向本地显示或 PC 上报：

- `arm_state`：布防/撤防状态
- `door_state`：门开/门关状态
- `alarm_state`：正常/报警及报警类型
- `connection_state`：A 板在线/离线状态

## PC 状态

PC 主要负责显示 B 板汇总的 `arm_state`、`door_state`、`alarm_state` 和 `connection_state`，同时提供控制入口和事件记录，不作为 A 板状态机的唯一状态来源。
