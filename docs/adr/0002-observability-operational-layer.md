# ADR-0002 · 补全运行级可观测层

- **状态**: Accepted
- **日期**: 2026-06-23
- **决策者**: A（架构师）
- **相关**: docs/observability.md；design.md §11；management.md §1.2/§2/§4.4/§9/§10
- **影响的 spec ID**: AC-1..5（观测支撑）、RULE-6 / CON-5（日志即出境面）、EDGE-5/6/8（排障）

## 背景

对 design.md §11 做客观评估：**验收/契约级可观测**（事件溯源、MetricsEngine、ContractGuard、DebtRegistry）完整；但**运行/工程级可观测**缺位——无结构化运行日志、无 RED/USE 运行指标、无跨进程关联追踪、无内部不变量断言体系、无健康/告警体系；且"日志/指标/追踪/异常是 RULE-6 出境面"未被统一治理（红线隐患）；团队层面缺可观测互通的单源管理。详见 observability.md §0。

## 决策

1. 新增运行级可观测设计（observability.md §1–§8）：结构化 JSON 日志、断言/不变量、RED/USE 指标、contextvars 关联追踪、健康检查/启动自检、告警分级——全部隐私安全。
2. 新增接缝 **K11**（可观测上下文与信号契约），单源 = `cloud/telemetry/obs.py` + 字段字典；全员只 import 本模块，保证无缝衔接。
3. 新增 CI 红线闸 `scripts/check_log_redaction.py`（RULE-6 的 DETECT），与 `obs.py` 脱敏过滤器（PREVENT）双层。
4. 登记 **DEBT-OBS-2**（轻量 contextvars 关联追踪而非完整 OpenTelemetry，OTel-ready）。

## 取舍

- **取**：运行期可排障可度量；日志/追踪/指标不再是红线漏洞；3 人模块经单源 obs.py 天然互通。
- **舍**：引入 obs.py 引导层与一道 CI 闸的前期成本；本轮不上全套 OTel（记债偿付）。对 demo 是正向取舍。

## 后果

- design v1 不改（保持冻结），本扩展以本 ADR + observability.md 形式登记。
- management.md 回填 K11、CI 新闸、§10/附录引用；组件 DoD 增"可观测四条"。
