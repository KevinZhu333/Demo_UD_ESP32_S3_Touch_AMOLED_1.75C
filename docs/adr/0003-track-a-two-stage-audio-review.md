# 复核备忘录 · ADR-0003「轨道 A 两阶段音频」影响评估

- **类型**: Review Memo（对 `docs/adr/0003-track-a-two-stage-audio.md` 的独立复核）；亦可充当 `management.md §8.4①` 所需 `contract-violation` Issue 的正文
- **日期**: 2026-06-24
- **复核人**: Claude Code（仓库默认复核 Agent）
- **被复核对象**: ADR-0003（评审快照口径：状态 Proposed，待 A 批准）
- **更新（2026-06-25）**: 被复核对象 ADR-0003 现已 **Accepted**（经 `management.md §8.4` 落地，Issue `#38` 已关闭、PR `#39` 已合并）。本备忘录是评审期历史快照，记录三轮复核意见，**不取代已落地的 ADR**；下文所有"待 A 批准 / 送 A 前"等措辞均为评审当时口径。规范决策以 `docs/adr/0003-track-a-two-stage-audio.md` 为准。
- **裁决**: ADR **方向正确、决策逻辑扎实**，但有 **1 处疑似错误的契约级决策（B3）+ 1 处会让 gate 隐形的命名（B1）+ 若干可执行清单遗漏（B2）+ 影响推理遗漏（B4/B5）+ 小项欠细（B6–B8）**。建议**补齐 B1–B3 后再送 A**；本备忘录不修改 ADR/spec/design/任何受 gate 文件，是否吸收交 A 与提案人。

---

## 0. 复核口径

按 `CLAUDE.md` 复核契约：以**特征代码、design.md、scripts/ gate、live 文件**为事实，不以文档自证。所有结论已逐条对实文件核验（附行号）。本备忘录是**纯 docs**、不入 gate；真正跑 gate 留待 A 批准后的单独实施 PR。

谬误本身（已确认，非材料误读）：spec/design 把轨道 A 定义为**单阶段**——提取主题后**提前一并生成**「约 3 分钟」音频 + 看板，**一次** 1-bit 推送，YES 播**预生成**音频。真实意图是**两阶段两推**：先推主题 → 接受 → **按需生成 10–15min** 音频 → 再推播放 → YES 播放。

---

## 1. 核验通过项（不需改，给 A 信心）

| 项 | 结论 | 证据 |
|---|---|---|
| ADR 行号引用 | 准确，确为契约本身 | design.md:413（`GENERATING(board ∥ audio) → READY`）、145-182（音频推送前在管线内生成）、407/421（NotebookLM 叶子依赖 / 看板必成、音频非必要） |
| §8.4 清单点名的 8 个 design.md 小节 | 方向正确，确承载单阶段假设 | §1.3/§1.4/§3.2/§3.3/§3.4/§11/§11.2/§10.2 |
| `PARAM-AUDIO_LEN` 拟新增行 | 与 `Param(...)` 签名完全吻合 | config/params.py:19-25 的 `Param(spec_id, default, bound, role, consumer)`；拟增 `Param("PARAM-AUDIO_LEN", 12, (5,20), "behavior", "Track-A 音频生成")` 合法 |
| K4/K6/K8 ↔ T0.4/T0.6/T0.8 映射 | 正确，三者当前 🟡（M0 冻结前吸收成本最低的时机判断成立） | docs/tasks/T0.4/T0.6/T0.8-*.md |
| 演示材料先行对齐 | slide 7/8/12/25/27 确已反映两阶段（按需音频、两推、10–15min） | presentations/demo-kickoff.html:272-302/306-347/430-476/929-956/977-992 |
| D4/D5/D6 + AC 链重评 | 逻辑自洽，AC 重评小节**已存在**（非缺失） | ADR §「验收链重评」§「已拍板事项」 |

---

## 2. 发现的问题

### B1 ·〔高 / 阻断〕`FR-A4a/FR-A4b` 命名对可追溯工具链隐形

ADR §8.4「1) spec.md / 4) traceability.yaml」拟把 `FR-A4` 拆为 `FR-A4a`/`FR-A4b`。但检查器解析不到带小写后缀的 ID：

- ID 正则 `ID_RE = \bFR-[A-Z]\d+\b`（scripts/check_traceability.py:30）匹配不到 `FR-A4a`。
- 矩阵键解析 `TOP_KEY_RE = ^([A-Z][A-Z0-9_]*-[A-Z0-9_]+):$`（同文件:39）破折号后只认大写，`FR-A4a:` 键也不会被解析。

复核期实测（一次性脚本）：

```
ID_RE.findall("FR-A4")  -> ['FR-A4']      ID_RE.findall("FR-A7")  -> ['FR-A7']
ID_RE.findall("FR-A4a") -> []             ID_RE.findall("FR-A4b") -> []
TOP_KEY_RE("FR-A4:")    -> FR-A4          TOP_KEY_RE("FR-A7:")    -> FR-A7
TOP_KEY_RE("FR-A4a:")   -> None           TOP_KEY_RE("FR-A4b:")   -> None
```

**后果**：两个新需求对 H2 **隐形**——`check_traceability.py --strict` 仍绿，但 `FR-A4a/FR-A4b` 既不被当作 spec ID、也无法在矩阵登记，**脱离可追溯/测试绑定**，违背仓库治理前提（CLAUDE.md「确认实现的 RULE/EDGE/AC 已用真实测试路径替换 `tests:[TODO]`」赖 ID 可被追踪）。

**修法（已与提案人定）**：改用**工具链兼容 ID**——`FR-A4` 保留 = **主题提议**（沿用 spec.md:103/134、design.md:15/176/466/967、traceability.yaml:88 的现有全部引用，零 dangling），新增 `FR-A7` = **播放提议**（A 轨下一个空号，纯大写无后缀，H2 可追踪）。ADR §8.4 的 FR-A4a/FR-A4b 措辞应整体替换为「保留 FR-A4 + 新增 FR-A7」。
> 备选（不推荐）：扩展 `ID_RE` 与 `TOP_KEY_RE` 接受后缀——需改 gate 脚本并回归，成本与风险都更高。

### B2 ·〔高〕修订清单漏掉的 design.md 单阶段落点

ADR §8.4「2) design.md」只点名 8 个小节，下列落点**未列入**，实施 PR 会照漏：

1. **design.md:466** 钩子文案 ——「想花 **3 分钟**听 X」(3-min 硬编码) + `FR-A4` 引用 + 论证「**不**复用 NotebookLM artifact（…会把**主题提取也绑死在脆弱依赖**上）」。后一句**与 ADR 拍板1（主题提取交给 NotebookLM）直接矛盾**，必须删改，否则 design 自相冲突。
2. **design.md:15**（§0 设计理念）引用 `FR-A4`——保留 FR-A4 方案下此引用仍成立（无需改），但若 A 改采任何重命名则会 dangling，需在备忘录提示。
3. **design.md:967**（design 内部 ID→章节覆盖表）`OUT-1 / FR-A4 / FR-B3 | §2.3…`——播放提议新增 FR-A7 后，此表应补 FR-A7 行/章节映射。
4. **§5.4 编排环（604-628）** `on_window_open` 单窗单卡、`Yes` 走单一 `fulfillment.trigger`——缺「topic 候选 YES → 触发音频生成（`AUDIO_REQUESTED`）」分支与顺序两候选编排。ADR D2 概念上已决，但**文件级编辑清单未含此处**。
5. **§6.1 反馈流（645-651）** `Yes→emit fulfillment.trigger` / `No→suppress` 对所有候选统一——需按 `kind` 分支（topic YES=触发生成、audio YES=播放；topic No=EDGE-1 不重弹、audio No=回 READY 有限重议）。ADR D6 概念上已决，**清单未含 §6.1**。
6. **§2.3 协议 `CARD_PUSH`（362-363）** 无 `push_kind` 字段；`audio_ready:true` 写死——对 topic_offer 卡语义错误（主题卡尚无音频）。ADR §11 提到 `PUSH_SHOWN 带 push_kind`，但**设备协议层 CARD_PUSH 的字段改动未列入**。

> 已排除的误报：design.md:176/413/421/863 分别属 §1.4/§3.2/§11，**已**在 ADR 清单内；**§7 Notion 仅服务轨道 B（封板 action-items），与轨道 A 看板/音频无关，不受影响**。

### B3 ·〔高 · 最深层〕关键路径回归未被识别

现设计**刻意**把 NotebookLM 挡在关键路径外，以保 AC-1：

- design.md:407「NotebookLM 是叶子依赖，不是骨架……管线在 NotebookLM 完全不可用时仍能产出**看板**」。
- design.md:421「**看板是 READY 的必要条件，音频不是**……把 NotebookLM 脆弱性挡在关键路径之外，保住 AC-1」。
- design.md:466 明确**反对**把主题提取绑死 NotebookLM；§1.4:165 现状是「LLM 主题/要点/钩子提取」走**内部 LLM**。

ADR（D2 +「看板 up-front 必成：看板在主题提取后立即生成」+ 拍板1「主题提取交给 NotebookLM」）把主题提取置于 NotebookLM 之上，而**看板下游依赖主题** ⇒ NotebookLM 挂 ⇒ 无主题 ⇒ **无看板 ⇒ 无推送**。这把脆弱依赖搬上了第一推的关键路径。

ADR 只把「依赖加重」当**可替换性**问题处理（DEBT-2/R2、Provider 抽象、唯一出境点），**未当可用性/关键路径回归处理**，且 §3.5 降级阶梯只覆盖音频、**无主题提取降级阶梯**。结果：现有「看板必成 / AC-1」承诺在新流程下不再成立，除非补 fallback。

**要求 ADR 二选一并记账**：
- **(a)** 为主题提取补降级阶梯（NotebookLM 主题 → **内部 LLM 主题** fallback；把 §3.5 阶梯延伸到主题层），从而保住「看板必成 / AC-1」；或
- **(b)** 显式**收回**「看板必成 / AC-1 不依赖 NotebookLM」承诺，记为新 DEBT 并在 AC-1 重评中明确接受该可用性下降。

这是送 A 前**必须澄清**的契约级取舍，建议作为 B1 之外的第二个 gating 项。

### B4 ·〔中〕FR-G2 吞吐后果未分析

两阶段每主题**两推**，D5 又（正确地）令 topic/audio/Track-B 合并计入 `PARAM-COOLDOWN`/`PARAM-DAILY_CAP`（默认 cap=6）。直接后果：**每日最多约 3 个主题**能走完两阶段。ADR 做到「不扩容打扰预算」值得肯定，但**未点出吞吐减半**对 AC-1（A 轨端到端）与 demo 体验（候选可能 TTL 过期、第二推长期抢不到窗口）的影响。建议进 ADR「取舍」与 AC-1 重评。

### B5 ·〔中〕二次推送对 FR-D2/RULE-4「禁延展」缺显式辩护

`audio_play` 第二推必须是**新的 idle+节流门控**推送，而非 topic 卡的自动延续——否则触 FR-D2「禁止自动加载下一条 / 继续看 / 猜你想看」与 RULE-4。ADR 流程图标了「[空闲+节流] 推送②」、设计上合规（design.md:628 schema 层无 `next_card`），但 AC-3 重评只写「RULE 集不变」，**未显式论证第二推为何不违 FR-D2**。建议在 AC-3 重评补一句结构性论证（第二推走独立窗口 + 独立节流，非链式延展）。

### B6 ·〔低〕参数计数注释漂移

config/params.py:11 `# PARAMSTORE-COMPLETE … 7 个 PARAM`：新增 `PARAM-AUDIO_LEN` 后应同步为「8 个 PARAM」。H3 按程序计数（同文件:103）不会因此变红，仅文档漂移，但应一并列入实施清单。

### B7 ·〔低〕DEBT-2 修订欠具体

ADR「6) debt/registry.py」只说「补…」，未定**落在哪个字段**（`summary` vs 新增 `note`）与**确切文本**。现状 debt/registry.py:31-36 `DEBT-2.summary = "依赖第三方加工能力 (NotebookLM 非官方/易 break)"`。R2 缓解项的**落点亦未点明**（演示风险表 slide 27 已更新，但代码侧/注册表 R2 位置未指明）。实施 PR 前应把确切 before/after 文本写死。

### B8 ·〔低 / 流程〕未链 contract-violation Issue

`management.md §8.4①` 要求 ADR + `contract-violation`/`task` Issue 同提；ADR 仅写 `Refs: T0.4/T0.6/T0.8`。建议据本备忘录建 Issue，并在 ADR 状态行前置「待建 Issue」。

---

## 3. 给 A 的建议（落地顺序）

1. **gating（送 A 前必办）**：
   - B1 — ADR §8.4 的 `FR-A4a/FR-A4b` 改为「保留 `FR-A4`（主题提议）+ 新增 `FR-A7`（播放提议）」。
   - B3 — ADR 增加主题提取可用性决策：(a) 主题降级阶梯，或 (b) 显式收回 AC-1/看板必成承诺 + 记 DEBT。
2. **应补（送 A 前宜办）**：B2 的 6 个 design.md 落点补进 §8.4「2) design.md」清单；B4/B5 补进取舍与 AC-1/AC-3 重评。
3. **可随实施 PR 办**：B6 计数注释、B7 DEBT-2/R2 确切文本、B8 建 Issue。
4. A 批准后单独发实施 PR（标 Contract Change，A 必审），届时跑全套 Required Local Checks，并确认 `PARAM-AUDIO_LEN` 在 spec §11.2 / config/params.py / docs/traceability.yaml 三处一致（H3）。

---

## 4. 复核证据附录

- B1 正则实测：见 §2.B1 代码块（一次性脚本，`re` 标准库）。
- B2 命中自检：`grep -rn "FR-A4\|3 分钟\|GENERATING\|CONTENT_READY\|audio_ready" docs/ presentations/`（排除 ADR 本体与 demo-kickoff.html:937「约 3–4 周」工期误报）——命中 spec.md:103/133/134、design.md:15/176/363/413/421/466/863/967、traceability.yaml:88，逐条已在 §1/§2 归类（已列入 ADR / 漏点 / 误报）。
- B3 依据：design.md:407/421/466/165 + ADR D2 / 拍板1。

---

# 第二轮复核（Round 2 · 对修订后 ADR 的再审核）

- **日期**: 2026-06-24
- **方法**: 多 agent 工作流（8 个核验 B1–B8 是否真落实并与 live 文件一致 + 5 个校验新事实声明 + 5 个对抗式 hunt + 1 个综合），结论已就三处关键项人工复核固化。
- **裁决**: 修订**方向正确、显著优于初稿**，B1–B6 已**解决并与 live 文件一致**；但**尚不足以直接送 A**——B7/B8 仍部分、对抗式 pass 又挖出 1 个**高/阻断**契约缺口（EDGE-6 耐久性）+ 2 个**高**（NotebookLM 主题提取属多余范围 / 删除 design.md:466 论证而非反驳）+ 若干中低项，并发现 1 处**必须先改的错误指令**。

## R2-1 · B1–B8 落实复核

| # | 状态 | 结论 |
|---|---|---|
| B1 | ✅ 解决 | `FR-A4a/FR-A4b` 已全面改为「保留 FR-A4 + 新增 FR-A7」(ADR:8/73/94/109-110/139/166)，a/b 仅存于"否决"语句。已对 `check_traceability.py:30/39` 实测:FR-A7 命中、FR-A4a/b 不命中。FR-A7 确为空号(A 轨止于 FR-A6)。 |
| B2 | ✅ 解决 | 5 个 design.md 漏点均入 §8.4 且各对应 live 章节:§2.3(:126→design.md:362-363)、§5.4(:127→604-628)、§6.1(:128→643-651)、:466 文案+矛盾论证(:125→466)、§B 映射(:132→967)。 |
| B3 | ✅ 解决 | 主题提取降级阶梯已写入(:49 `NotebookLM→internal LLM→静默重试/丢弃`，:48/81/97/124/158/176 呼应)，重新解耦看板与 NotebookLM 可用性。**但**这把"今天的可靠主路径(内部 LLM, design.md:165)降级为 fallback"——见 N-NLM-SCOPE,其实是循环修复。 |
| B4 | ✅ 解决 | 吞吐后果已写(:84 每日约 3 主题)。残留(非阻断)::84/63/116 以"候选 TTL 衰减"为泄压阀,但 config/params.py 无 TTL 参数、§8.4(3) 也只加 PARAM-AUDIO_LEN——所引机制未落地。 |
| B5 | ✅ 解决 | 第二推对 FR-D2/RULE-4 的独立窗口辩护已入 :85 与 AC-3 重评 :179。残留→见 N-RULE2(RULE-2 角度未单独辩护)。 |
| B6 | ✅ 解决 | §8.4(3) 已含「7→8 个 PARAM」(:136),与 params.py:11 字节匹配。 |
| B7 | ⚠️ 部分 | 字段/文件歧义已消(item 6 点名三个真实字段、item 7 钉到 management.md:511)。但只有 `DEBT-2.summary` 给了 verbatim before/after(:147);`surfaced_by`(:148)/`repay_before`(:149)/R2(:152) 仅方向性描述,未引 live 原文。**并含一处错误指令**(见 R2-3)。 |
| B8 | ⚠️ 部分 | Issue 已作为 A 批准前置且在落地顺序前排(:105/182),但仍未真正建/链 Issue(仅 `Refs: T0.4/T0.6/T0.8`),状态行(:3)也未按建议加「待建 Issue」。流程项,可随实施 PR 办。 |

## R2-2 · 新发现（修订引入或前轮未覆盖）

- **N-EDGE6 ·〔高/阻断〕EDGE-6 耐久性未覆盖新异步窗口**。两阶段把 topic YES 变成"已确认但未完成的履约"=触发**异步**音频生成(ADR:47/120/127/129),正落 EDGE-6「不得丢失已确认(Yes)未完成的履约,恢复后补完」(spec.md:177)。但 ADR EDGE 范围只列 EDGE-1/3/5(:8/116),EDGE-6 零提及;现有恢复机制(§7 outbox+sweeper, design.md:684-695)是 **Notion 写专用**,单阶段 FSM 的 `ACCEPTED→FULFILLED`(design.md:132/143)当年是为"播放已生成音频"写的,中间没有慢异步生成步可依赖。崩溃→用户双击 Yes 可能被静默丢弃。EDGE-5(静默丢弃)≠EDGE-6(绝不丢、补完)(design.md:136 vs 137)。**建议作为送 A gating**:把 EDGE-6 加入受影响 ID 与 §9 澄清;为 `AUDIO_REQUESTED→AUDIO_READY` 指定持久化/幂等/sweeper 重驱;补 AC-1 崩溃恢复与测试落点。
- **N-NLM-SCOPE ·〔高〕把主题提取搬上 NotebookLM 属多余范围,正是它制造了 B3 回归**。用户真实意图(ADR:22-24)只含三项音频侧 delta(预生成→按需、单推→双推、3min→10-15min),**无一要求换主题提取引擎**。live 设计本已分离:design.md:165 主题/要点用**内部 LLM**、NotebookLM 仅做音频(design.md:169)。把主题提取改到 NotebookLM 才把脆弱依赖推上第一推关键路径、破坏 design.md:407/421 的"看板必成/AC-1";而 B3 的 fallback 恰恰就是今天的内部 LLM 主路径——等于"降级今天的可靠主路径→提升脆弱依赖→再把旧主路径当净零安全网",还附带 Provider 抽象/阶梯/测试/DEBT 加重。N:M 的理由不为 NotebookLM 独有,若暗示"NotebookLM 主题质量更高"则撞 RULE-11/N5(质量非目标,spec.md:163/35)。**建议作为送 A 首要决策**:解耦二者——采纳两阶段音频,但**主题提取保留在内部 LLM**(design.md:165 不动)、NotebookLM 只做按需音频,从根上消除关键路径回归,并省去主题阶梯/新 Provider/额外 DEBT/删 design.md:466。
- **N-D466 ·〔高〕计划"删除"design.md:466 的论证而非反驳它**。design.md:466 明确记载了一条有理由的设计立场(不复用 NotebookLM artifact,因"会把主题提取绑死在脆弱依赖上"),正是 ADR 拍板1 的反面。ADR 仅安排"删除该旧论证"(:125),未给出"为何该风险如今可接受"的实质反驳(只指向 fallback,而 N-NLM-SCOPE 已证其循环)。删除一条有据的设计不变量来迁就其反面=设计完整性回归。**建议**:要么采纳 N-NLM-SCOPE 保留 :466,要么在 ADR 正文给出实质反驳。
- **N-EST-MINUTES ·〔中〕§3.4 数据模型遗漏 `topics.est_minutes`**。该列(design.md:456)原存单阶段"~3min",改后时长由 PARAM-AUDIO_LEN(默认 12,[5,20])承载,形成**双真相源**。§8.4 §3.4 项(:123)只改 PK/FK+N:M,未提此列。**建议**:明确处置——删列让 PARAM-AUDIO_LEN 单源,或保留为"由 param 派生的快照"并定同步规则。
- **N-NM-FALLBACK ·〔中〕内部 LLM fallback 无法结构性保证 N:M 跨文合并**。pipeline 与数据模型按**单文档**(design.md:413、topics document_id PK :455);NotebookLM 宕机时 fallback 只能退化为逐文档主题(至多 1:M 拆分,绝无 N:1 合并),而 ADR(:35/176)把它当等价连续性路径、且只把 fallback 影响说成"质量"(:81)。**建议**:声明 fallback 是否需复现 N:M(需则补多文档批处理+topic-scoped 迁移),否则记为可接受的非等价 DEBT;把 fallback 从 :35 的 N:M 括号里挪出。
- **N-EDGE8 ·〔中〕N:M 主题 + 每主题一卡使 EDGE-8 被新触压而未覆盖**。近似/重复主题会触发多张 topic_offer,违 EDGE-8「去重合并,不重复推送同一件事」(spec.md:179);现 live 仅 Track-B 有去重(design.md:202/499/544)。**建议**:为 Track-A topic 级去重补 §8.4 项+EDGE 澄清+测试落点。
- **N-RULE2 ·〔中〕coupled 双推未对 RULE-2 单独辩护**。ADR 严辩 FR-D2/RULE-4 却未论证"同一主题先问后问"为何不算 RULE-2 禁止的预问/多轮(spec.md:154)。**建议**:在 AC-3 重评显式说明 topic_offer 与 audio_play 是两次独立 1-bit 决策(各自 idle 门控、只收 Yes/No/ABORT、设备端不留会话态),否则交 A 作为 RULE-2 契约问题。
- **N-EDGE35-UNBACKED ·〔低〕** EDGE-3/5 澄清只在 §8.4 出现、无 D 段决策背书(D1 happy-path 无生成失败/饥饿分支)。建议在 D 段补一句决策。
- **N-FRA5-FWDREF ·〔低〕** FR-A7 编号在 FR-A5 之后,致 spec 重写后 FR-A5「播放卡 YES 后」前向引用尚未定义的 FR-A7。gate 不关心(纯 cosmetic),建议重写时让 FR-A5 显式交叉引用 FR-A7。

## R2-3 · 必须先改的错误指令

- **〔必改〕ADR:148(DEBT-2.surfaced_by)**:指令"补『…失败计 EDGE-5 不报错』",但该子句**已存在**于 live `registry.py:34`(`Provider 抽象 + 三档降级阶梯; 每次调用 emit DEBT_EXERCISED; 失败计 EDGE-5 不报错`)。照做会**重复**。应:引 :34 verbatim 作 before,仅新增真正新增的子句(`主题提取有 internal LLM fallback；按需音频仅对已接受主题调用`),删掉冗余的 EDGE-5 子句。
- **〔宜改〕ADR:149(repay_before)** before 为转述,非 live 原文(registry.py:35 `替换为可控/自研音频生成, 保持 Provider 可热插拔`)——按"钉死 before/after"标准应引 verbatim。
- **〔宜改〕ADR:7 / :105** 的"相关/不直接修改"清单遗漏 `debt/registry.py` 与 `docs/management.md`(均被 §8.4 item 6/7 修改);且 :7 的 design.md 章节清单(§1.3/§1.4/§3.2/§3.3/§11)漏列 §8.4 实际触及的 §2.3/§3.4/§3.5/§5.4/§6.1/§10.2/§11.2/§B。建议补齐使"声明的编辑面"与"§8.4 计划"一致。

## R2-4 · 送 A 前建议

1. **gating（先办）**:N-EDGE6(EDGE-6 耐久性) + N-NLM-SCOPE/N-D466(是否解耦主题提取,首要决策) + ADR:148 错误指令必改。
2. **应补**:N-EST-MINUTES、N-NM-FALLBACK、N-EDGE8、N-RULE2 进 §8.4/AC 重评。
3. **可随实施 PR**:B7 余下 verbatim、B8 建 Issue、N-EDGE35/N-FRA5、:7/:105 清单补齐、B4 的候选 TTL 参数补定。

---

# 第三轮复核（Round 3 · 对第三次修订的再审核）

- **日期**: 2026-06-24
- **方法**: 回归矩阵(13 项 Round-2 条目) + 4 个对抗式 lens(隐私/留存、共源理据、gate 交互、完整性) + 综合;两处可证伪的关键新发现(F3 AC-1 度量行、F6 既有 CAND_TTL)已人工对 design.md:555/845/869 复核固化;debt/registry.py 与 management.md R2 的 before 串已人工**逐字节核对一致**。
- **裁决**: `ready_with_minor_fixes` —— **Round-2 全集已闭环**(含四处错误指令全部改为逐字 verbatim、surfaced_by 重复 bug 已消)。但**第三轮新增的 D8(音频请求耐久性 + topic-scoped 表重建 + 新 audio_requests 表 + 新事件)自身**带出一簇新缺口:新持久化层未接入仓库既有的「留存/隔离/脱敏」三条底座。两项触及 CON-5③/RULE-6 红线、一项是 D8 自身依赖的 AC-1 接线 —— 建议在送 A 前补进 §8.4;其余为收紧文本项。

## R3-1 · Round-2 回归(全部已处理)

B7 ✅(三处 DEBT-2 + R2 before 全 verbatim、重复已消) · B8 ⚠️部分(状态行+落地顺序已点,Issue 仍未真正建/链——流程项可延) · N-EDGE6 ✅(D8 强制持久化 AUDIO_REQUESTED + sweeper at-least-once 重驱;EDGE-6 入 IDs/§9/数据模型/AC-1/测试;EDGE-5≠EDGE-6) · N-NLM-SCOPE ⚠️部分(质量理据已撤、非等价已声明,但"解耦"被明确拒为 kept-by-choice A 决策——理据问题见 F4) · N-D466 ✅(改为改写/反驳而非删除 + 拍板3) · N-EST-MINUTES ✅单源 mandate(D3/57/107),**但 §8.4:141 引入新矛盾**——见错误指令① · N-NM-FALLBACK ✅(明确非 N:M 等价、移出 N:M 框、测试断言标记降级) · N-EDGE8 ✅(Track-A topic 去重入 D1/D2/D9/后果/§9/测试) · N-RULE2 ✅(D9+AC-3+§8.4 辩护两次独立 1-bit) · N-EDGE35 ✅(D6/D8/D9 已带决策) · N-FRA5 ✅(FR-A5 交叉引用 FR-A7) · B4-TTL ⚠️部分(条件化 PARAM+H3,但仍依赖未登记 valve 且与既有 CAND_TTL 冲突——见 F6) · 四处错误指令 ✅全部改为 verbatim 且正确。

## R3-2 · 新发现（均由第三轮 D8 新增面带出）

- **F1 ·〔高/红线〕新持久层逃逸 CON-5③ 留存底座**。D8 的 `audio_requests` 检查点 + 重建的 topic-scoped topics/boards/audio 表均无 `purge_after`/Reaper TTL。live `documents.purge_after TIMESTAMPTZ NOT NULL`(design.md:452)、`data_assets` expires_at(design.md:789)是限期清理底座,T0.8 WI-2 验收点正是「CON-5 限期清理字段存在」;§8.4:141 只列 PK/FK+N:M+幂等重驱+est_minutes,**未要求新/重建表带 purge_after**。叠加 N:M(一 topic 映射多 document,级联删除路径变歧义),新表可能落在限期清理外 → 违 CON-5③(红线)。**建议**:§8.4 §3.4 + T0.8 落地点写明新表进入 `purge_after`/Reaper,并在 AC-5/验收链加一行。
- **F2 ·〔高/红线〕新/重建表未接入 RULE-6 隔离底座**。RULE-6 物理隔离靠 `documents.origin='track_a' CHECK`(design.md:453)+ 行级 tenant RLS(design.md:789),T0.8 WI-3 验收「RULE-6 物理隔离底座可验」。§8.4:141 与后果 K8(:107)未要求重建的 topic-scoped 表与 `audio_requests` 带 origin CHECK / 入 RLS → D8 数据层 RULE-6/租户隔离留口。**建议**:§8.4 §3.4 与 K8 写明新表入 tenant RLS + 带 origin='track_a'(或等价 Track-A 归属护栏)。
- **F3 ·〔高〕§8.4 漏掉 AC-1 度量行(design.md:869),D8 的 EDGE-6 音频恢复无度量落点**。已核:design.md:869 AC-1 行 `含 EDGE-6 迟到补完；旁路 recovery_rate` 仅 **Track-B outbox 口径**,Track-A 只有 `e2e≥1 + content_gen_success`。§8.4 的 §11.2 项(:147)只改 AC-2 push_kind、§11 项(:146)只加事件,**未编辑 AC-1 行**;AC-1 叙述重评(:207)非文件级指令。照做则 D8 的 Track-A AUDIO_REQUESTED 恢复(D8 所依赖的机制)无 AC-1 度量落点。**建议**:§8.4 加一条编辑 design.md:869,把 EDGE-6 迟到补完/recovery_rate 扩到 Track-A AUDIO_REQUESTED→AUDIO_READY/终态。
- **F4 ·〔中〕共源理据不成立且与 live 即用即删/SessionPool 模型冲突**。ADR:51/95「topic/audio 共源」并不能确立 topic↔audio 一致性:一致性来自把音频生成**条件化于已接受 topic/文档**(`generate(doc: TrackADocument)` design.md:434、`clean_text` :427),与谁提取 topic 无关;且 T1 提取/T2 生成时序分离,live「音频下载后立即 delete_notebook」(design.md:440)+ SessionPool 按 user_id 而非 topic,**跨 T1→T2 无共享 notebook 会话**。理据仍像被包装成链路风险的质量主张(RULE-11/N5)。kept-by-choice 决策可留,但**理据应重写**:落到"条件化于已接受 topic"这一真实机制;若确要共享会话,须把"跨接受边界保留 notebook 会话"列为显式假设、标注偏离 design.md:440,并把验证它写成 T0.16 spike 的明确目标。
- **F5 ·〔中〕新事件/错误字段缺 RULE-6 内部限定**。D8 的 `AUDIO_REQUESTED/TOPIC_READY/AUDIO_READY` 事件与 `audio_requests.last_error`/terminal/审计字段无「仅内部、无原文」约束;live `audio_artifacts.last_error -- error 仅内部`(design.md:461)、`check_log_redaction.py` 把 transcript/clean_text/raw_quote 等落日志判为出境红线。**建议**:§8.4 §3.4/§11 为新错误/审计字段与新事件 payload 加 RULE-6 行(只存结构化关联 + error code、error 仅内部、过 obs.py 脱敏单源),并加测试断言新事件 payload 不含敏感 token。
- **F6 ·〔中〕audio_play stage-2 TTL 与既有 PARAM-CAND_TTL(18h,不跨业务日)冲突**。已核:候选 TTL 参数**已存在**——design.md:555 `now ≥ created_at + PARAM-CAND_TTL(18h, 不跨业务日) → EXPIRED`、design.md:845 内部参数 `CAND_TTL（18h）`。但 §8.4(3)(:154)把候选 TTL 当**尚不存在**的新参数处理;且「不跨业务日」语义与 stage-2「次日清晨通勤窗再议」前提冲突(晚间生成的 audio 候选会在次晨前过期)。**建议**:§8.4 改为**与既有 CAND_TTL 调和**——决定 stage-2 再议是否可跨业务日,据此改 CAND_TTL 语义或引 stage-2 专用界并说明与 CAND_TTL 关系,而非新造参数。
- **F7 ·〔低〕demo-kickoff slides 对第三轮 EDGE-6 已过时**。ADR:115 称 slides「已先行修正」,但只反映音频侧 delta,未含第三轮 EDGE-6:M2 Track-A 出口仅列 `降级阶梯(EDGE-5/10)`(demo-kickoff.html:944)、Track-B 流程有 `EDGE-6 离线 YES…补完`(:374)而 Track-A 流程无 EDGE-6 红线。**建议**:更新 slides 或把"已修正"声明限定到音频侧并把 slides 加入 §8.4 后续编辑清单。

## R3-3 · 错误指令（须先改）

- **① ADR:141(est_minutes 处置)自相矛盾 + 读序倒置**。§8.4:141 建议把 `topics.est_minutes` 迁为「`audio_artifacts` 派生快照(带 param_version)」,但 `audio_artifacts` 行**只在接受后生成完成才存在**(ADR:42),而任何在 stage-1 topic 卡(ADR:39)上要显示的时长在音频存在前就需要 → 指向 audio_artifacts 造成**读序倒置**;且与 ADR:57/107「est_minutes 不得成第二真相源/由 PARAM-AUDIO_LEN 派生」矛盾。注:新两卡文案(:39/:43)其实都不显示时长,故最干净是**删 est_minutes、PARAM-AUDIO_LEN 单源**。**应改**:统一 :141 与 :57/107,按"哪张卡哪个阶段显示什么时长"决定存储归属(不显示→删;渲染期显示→从 PARAM 派生;另设字段存生成后真实时长)。
- **② ADR:154(候选 TTL 参数化)** 见 F6:把"假想新参数"框架改为"与既有 PARAM-CAND_TTL 调和"。

## R3-4 · 送 A 前建议

1. **gating(先办)**:F1+F2(新持久层补 `purge_after` + origin/RLS,CON-5③/RULE-6 红线) + F3(§8.4 补 AC-1 度量行编辑) + 错误指令①(est_minutes 矛盾)。
2. **应补**:F4(重写共源理据 / 给 T0.16 spike 派活)、F5(新事件/错误字段 RULE-6 脱敏)、F6+错误指令②(调和 CAND_TTL)。
3. **可随实施 PR / 流程**:B8 建 Issue、F7 slides、B4 残留。
4. **元观察**:每轮修订把更细的设计细节(本轮 D8 整个持久化层)塞进 ADR,因而每轮带出一簇更小的新缺口;F1/F2/F3 同源——都是"新 D8 面须继承既有 隐私/度量 底座"。补齐这一主题后即可送 A。

## R3-5 · §8.4 补丁清单（可粘贴；留痕用，未落进 ADR）

> 覆盖 F1/F2(新表 `purge_after`+origin/RLS)、F3(AC-1 行编辑)、est_minutes 单源、F6/错误指令②(CAND_TTL 调和)及配套测试。粘贴落点与 before/after 已对齐 live 行号(design.md:452/453/555/845/869)。本节仅在备忘录留痕,**ADR 由用户维护**。

**补丁 1 — §8.4「2) design.md · §3.4 数据模型」(替换 ADR:141 整条)**

before:
```
- **§3.4 数据模型**：topics/boards/audio_artifacts 以 topic 为主键或外键；增加 document-topic N:M 映射；candidates 以 kind/parent_id 串联两阶段；增加 audio_requests 或等价 job/outbox 表承载 AUDIO_REQUESTED 幂等重驱；处理 topics.est_minutes，推荐删除或迁移为 audio_artifacts 派生快照（带 param_version），避免与 PARAM-AUDIO_LEN 双真相源。
```
after:
```
- **§3.4 数据模型**：topics/boards/audio_artifacts 以 topic 为主键或外键；增加 document-topic N:M 映射；candidates 以 `kind`/`parent_id` 串联两阶段；增加 `audio_requests` 或等价 job/outbox 表承载 `AUDIO_REQUESTED` 幂等重驱。**新增/重建的全部 topic-scoped 表与 `audio_requests` 必须继承既有隐私底座**：
  - ① **CON-5③ 限期清理**：每行带 `purge_after`（或纳入 Retention Reaper 的 `expires_at`/TTL 路径），与 `documents.purge_after`（design.md:452）、`data_assets.expires_at`（design.md:789）同口径；N:M 下须明确 topic 的清理触发（不得因一对多而漏清），满足 T0.8 WI-2「限期清理字段存在」。
  - ② **RULE-6 物理隔离**：带 `origin='track_a' CHECK`（或等价 Track-A 归属护栏，对照 `documents.origin` design.md:453）并纳入行级 tenant RLS（design.md:789），满足 T0.8 WI-3「物理隔离底座可验」。
  - ③ **est_minutes 单源**：两阶段两卡（「想了解《主题》吗?」/「音频好了，现在听?」）均不显示时长，故**删除 `topics.est_minutes`**，`PARAM-AUDIO_LEN` 为时长唯一真相；若后续某卡需显示预计时长，则渲染期由 `PARAM-AUDIO_LEN` 派生（参数读），另设 `audio_artifacts` 字段仅存生成后**实际**时长（不同生命周期），不得让 `est_minutes` 成第二真相源（呼应 D3 / 后果 K8）。
```

**补丁 2 — §8.4「5) docs/task.md · M0」T0.8(替换 ADR:161 中 T0.8 一句)**

before: `…T0.8 加 topic-scoped 表形、document-topic N:M、audio_requests/outbox 与 candidates.kind/parent_id。`
after:
```
`T0.8` 加 topic-scoped 表形、document-topic N:M、`audio_requests`/outbox 与 `candidates.kind/parent_id`；验收点补「新表/重建表带 `purge_after` 并入 Retention Reaper（WI-2）」与「新表带 `origin='track_a'` CHECK + 入 tenant RLS（WI-3）」，使 CON-5③/RULE-6 底座对新持久层可验。
```

**补丁 3 — §8.4「2) design.md」新增 AC-1 度量行编辑(F3,插在 §11.2 项 ADR:147 之后)**
```
- **§11.2 AC-1 度量行（design.md:869）**：把 EDGE-6「迟到补完」/旁路 `recovery_rate` 从仅 Track-B Notion outbox 口径**扩展到 Track-A**：`topic_offer YES → AUDIO_REQUESTED → AUDIO_READY/明确终态` 的崩溃恢复补完必须计入 AC-1，与 D8 可恢复检查点对齐；写明 Track-A 恢复补完如何进入贯通判定，使 D8 恢复机制有度量落点（现 design.md:869 仅 `A 轨 e2e≥1 + content_gen_success`，不含 A 轨 EDGE-6 恢复）。
```
并在「验收链重评」AC-1 那条(ADR:207)句末追加: `；AC-1 度量行（design.md:869）须同步把 EDGE-6 迟到补完/recovery_rate 覆盖 Track-A AUDIO_REQUESTED 恢复，而非仅 Track-B outbox。`

**补丁 4 — §8.4「3) config/params.py」候选 TTL 调和(F6,替换 ADR:154)**

before: `- 若实现决定给候选 TTL 单独参数化，必须同步新增 PARAM 与 H3，而不能只在文档中引用未登记参数。`
after:
```
- **候选 TTL 不是新参数**：复用既有内部参数 `PARAM-CAND_TTL`（design.md:555/845，默认 18h、「不跨业务日」）。须在 design.md §4.5/§5（候选衰减/编排）与 §10.2 调和 `audio_play` stage-2 生命周期与 `CAND_TTL`：「不跨业务日」会让晚间生成的 audio 候选在次晨通勤窗前过期，与 D6/D9「下一合适窗口再议」冲突——明确决定 stage-2 再议**是否可跨业务日**：可，则改 `CAND_TTL` 语义（或为 `audio_play` 引 stage-2 专用界并写明与 `CAND_TTL` 关系），按「调数值不算改契约」走 §10.3 热生效；不可，则在 D6/D9 写明 audio 候选当日不取即过期的后果。任何新增可调界仍须登记并过 H3。
```

**补丁 5 — §8.4「8) 测试落点」(新增两条)**
```
- `tests/integration`：新持久层隐私底座覆盖——`audio_requests` 与重建的 topic-scoped 表带 `purge_after` 且被 Reaper 清理（CON-5③）；带 `origin='track_a'` + RLS，跨租户不可见（RULE-6）。
- `tests/contract`（AC-1 度量）：钉死 Track-A `AUDIO_REQUESTED` 崩溃恢复后补完计入 AC-1 的 EDGE-6 迟到补完/`recovery_rate`（不仅 Track-B）。
```

**配套头部微调(非必须)**: 影响的 spec ID(ADR:8)补 `CON-5`;「相关」(ADR:7)design.md 章节清单补 `§4.5`。
