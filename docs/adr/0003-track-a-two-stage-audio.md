# ADR-0003 · 轨道 A 改为「两阶段：先推主题、接受后按需生成音频」

- **状态**: Accepted（经 `management.md §8.4` contract-change 流程落地；总 Issue: #38）
- **日期**: 2026-06-24
- **提案人**: 启动会材料评审（用户指出真实产品意图与契约不符）
- **决策者**: A（架构师）— 本 ADR 标记为 **Contract Change**，本轮按 Issue #38 承接 A 评审与 AC 链重评
- **相关**: `docs/spec.md` §5.2/§6.1/§7/§8/§9/§11.2；`docs/design.md` §1.3/§1.4/§2.3/§3.2/§3.3/§3.4/§3.5/§4.5/§5.4/§6.1/§9.3/§10.2/§11/§11.2/§B；`docs/traceability.yaml`；`config/params.py`；`docs/task.md` §4(M0)/§6(M2)；`debt/registry.py`；`docs/management.md` §11/R2；`presentations/demo-kickoff.html`
- **影响的 spec ID**: FR-A3、FR-A4（保留为主题提议）、FR-A7（新增播放提议）、OUT-2、AC-2、FR-G2、PARAM-AUDIO_LEN（新增）、RULE-2、RULE-6、CON-5、EDGE-1/3/5/6/8（澄清）；间接 K4/K6/K8 接缝

---

## 背景

启动会材料把轨道 A 画成**单阶段**流程，经逐字核实，**这是 spec/design 契约本身的定义**，而非材料误读：

- `spec.md:102` **FR-A3**：「系统必须基于主题生成**音频对话与信息看板**」——音频与看板一并、提前生成。
- `spec.md:103` **FR-A4** + `spec.md:131-138` **旅程一** step 2–3：「提取主题与要点，**生成一段约 3 分钟的音频对话**与看板」→ **一次**空闲推送「想花 **3 分钟**听…吗？」→ YES 播放**已生成**音频。
- `design.md:413` Document 状态机 `EXTRACTING → GENERATING(board ∥ audio) → READY`；`design.md:145-182` §1.4 时序：音频在**管线内、推送前**生成（`PIPE->>NLM: 生成 Audio Overview`），候选 READY 后**一次** `push-card`，YES 即播放。
- `design.md:407/421`：「NotebookLM 是叶子依赖」「看板是 READY 必要条件，音频不是」——均假设音频**预生成**。

**真实产品意图（用户确认）**：
> 先只提取**主题** → 空闲推送主题 → 用户**接受**喜欢的主题后，才**按需生成 10–15 分钟**音频 → 生成完成后空闲**再推一次** → YES 播放。

差异是实质性的：(a) 音频从「预生成」改为「主题接受后按需生成」；(b) 单次推送改为**两次** 1-bit 推送（主题提议 / 播放提议）；(c) 时长从「约 3 分钟」改为「10–15 分钟」可调。

**为什么现在必须处理**：该变更触及 M0 接缝 **K4 候选契约（T0.4）**、**K6 事件契约（T0.6）**，并波及 **K8 DB（T0.8）**——三者当前 🟡、即将冻结。M0 冻结前吸收成本最低；冻结后改动须再走 ADR 重开。

---

## 决策

### D1 两阶段流程（轨道 A）
```
IN-1 转发 → 抓取/清洗(FR-A1)
  → NotebookLM 提取主题(N:M：可跨文合并/单文多主题；失败则 internal LLM degraded fallback，可退化为逐文档 topic)
  → 看板必成(轻量,up-front,topic-scoped)
  → topic 级去重/合并(EDGE-8)
  → 每个主题 = 一个【topic_offer 候选】READY
  → [空闲+节流] 推送① 主题 1-bit「想了解《主题》吗?」(OUT-1)
  → 双击 YES = 接受该主题 + 持久化 AUDIO_REQUESTED(EDGE-6, 幂等重驱)
  → 按需触发 NotebookLM 生成 10–15min 音频(PARAM-AUDIO_LEN，经唯一出境点，仅对被接受主题)
  → 音频 READY → 生成【audio_play 候选】READY
  → [空闲+节流] 推送② 播放 1-bit「音频好了，现在听?」
  → 双击 YES → 履约：耳机播音频(OUT-2)+手机看板(OUT-3)，可高亮批注(FR-A5/RULE-10)
  → 尽快熄屏(FR-D2) → (可选)MCP 续聊(FR-A6/OUT-5)
```

### D2 设计取向（最小化接缝改动）
- 两阶段建模为**两个顺序候选走同一「统一候选状态机」**（`§1.3`）：`Candidate` 加字段 `kind ∈ {topic_offer, audio_play}` 与 `parent_id`，**不新增状态**。topic 候选的「履约」= 触发音频生成；音频 READY 后派生 audio 候选进入同一 FSM 做第二次推送。
- **主题提取主路径交给 NotebookLM，但不把 AC-1 绑死在 NotebookLM 上**：NotebookLM 作为 demo 默认的 `TopicExtractionProvider` 与 `AudioProvider`；主题提取必须有 **internal LLM degraded fallback**。两者都藏在 Provider 抽象之后，经唯一出境点、SessionPool / 隔离 worker 池 / 即用即删约束，不把 NotebookLM 写成不可替换核心（CON-3/DEBT-2）。
- **明确接受并记录对 `design.md:466` 的偏离**：现有设计反对复用 NotebookLM artifact，理由是会把主题提取绑死在脆弱依赖上。本 ADR 仍选择 NotebookLM 主题主路径，是 demo 阶段对第三方加工路径与 Provider 接缝的显式取舍；音频生成必须条件化于已接受的 `topic_id` / document 集合，不能依赖跨「主题提取 → 音频生成」保留 NotebookLM 会话来保证一致性。若后续实现要共享 notebook 会话，须由 T0.16 spike 验证并标注对即用即删模型的偏离。该选择不是无风险删除旧论证，而是把风险转入 Provider 抽象、fallback、DEBT-2/R2 与 A 评审。
- **主题提取降级阶梯**：`NotebookLM topic extraction → internal LLM degraded fallback → 静默重试/丢弃`。fallback 只承诺可用性保底：至少产出可推送 topic 与看板；若无法复现跨文档 N:M 合并，则允许降级为逐文档 topic，并记入 DEBT/实现限制，不把 fallback 写成质量或结构等价路径。
- **文章↔主题为 N:M**，由主题提取 Provider 结果决定（多篇可并一主题、单篇可拆多主题）；候选生成前必须做 topic 级去重/近似合并（EDGE-8）；每主题各自一张 1-bit 卡——**无菜单、无枚举，符合 RULE-1/2**。
- **看板 up-front 必成**：看板在主题提取后立即生成并绑定 topic，播放时呈现；音频仍在 topic 被接受后按需生成。主题/看板/音频均从 document-scoped 调整为 topic-scoped，并保留 document-topic N:M 映射。

### D3 时长参数化
新增可调参数 **`PARAM-AUDIO_LEN`**（音频目标时长，默认 12 分钟，区间建议 `[5, 20]`，demo 目标带 10–15 分钟），`behavior` 类，受 H3 参数同步闸约束；替换所有「3 分钟」硬编码。数据模型不得再让 `topics.est_minutes` 成为第二真相源。

### D4 AC-2 全量打扰口径
- **所有主动推送都进入 AC-2 主分母**：`topic_offer`、`audio_play`、Track-B 推送均计入 `PUSH_SHOWN - ABORT`。第二推是用户注意力成本，不能因其来自已接受主题而从主指标中隐藏。
- **必须分层展示**：`PUSH_SHOWN` 带 `track` 与 `push_kind ∈ {topic_offer, audio_play, track_b_seal}`；MetricsEngine 同时输出整体与分层的 `yes_rate`、`non_annoyed_rate`、`composite_acceptance`。
- **ABORT 仍剔除分母**：ABORT 代表时机被打断，不代表内容或卡片被拒绝；§6 仍只发原始 feedback，AC-2 由 MetricsEngine 唯一计算。

### D5 FR-G2 合并节流
- `PARAM-COOLDOWN` 与 `PARAM-DAILY_CAP` 对同一用户**全局合并计数**，不按 topic/audio/Track-B 分池。
- `audio_play` 第二推不得绕过 cooldown 或 daily cap；若音频已生成但当天 cap 已满或仍处 cooldown，则 audio 候选继续等待下一合适窗口或按 TTL 衰减。
- 这样保留 FR-G2 的用户打扰边界：两阶段提升了表达力，但不扩张每日主动触达预算。

### D6 No/timeout 语义按候选类型分层
- `topic_offer No/timeout`：表示用户不接受该主题，按 EDGE-1 静默消失、不重复弹、不追问。
- `audio_play No/timeout`：表示「现在不听」，不等同于不喜欢该主题；audio 候选回到 `READY`，受全局 cooldown/cap、候选 TTL 与有限重议约束，后续可在更优空闲窗口再议。
- 两类 No 都计入 AC-2 的 `No`；是否触发 `ANNOY_SIGNAL` 仍由显式/启发式/访谈信号分层决定，不能用 `audio_play No` 自动推断用户反感内容。

### D7 工具链兼容 ID 命名
- 不采用 `FR-A4a/FR-A4b`：当前 `scripts/check_traceability.py` 只识别 `FR-[A-Z]\d+`，小写后缀会对 H2/H3 隐形，导致需求无法进入 `docs/traceability.yaml` 与测试账本。
- **保留 `FR-A4` = 主题提议**，沿用现有引用；**新增 `FR-A7` = 音频播放提议**，作为 A 轨下一个工具链兼容空号。

### D8 EDGE-6 音频请求耐久性
- `topic_offer YES` 后不能只在内存里触发异步生成；必须落库一个 `AUDIO_REQUESTED` / audio job 检查点，包含 `topic_id`、`parent_candidate_id`、`provider`、`audio_len`、存储型 `idempotency_key`、attempts 与 terminal state；幂等键按 `parent_candidate_id` 稳定派生，不能随 provider/audio_len 重算。
- `AUDIO_REQUESTED → AUDIO_READY` 由后台 worker + sweeper at-least-once 重驱；进程崩溃、设备离线、网络中断后恢复时，不得丢失「已确认 Yes 但未完成」的履约（EDGE-6）。
- `EDGE-5` 只约束「不向用户报错、失败可降级/重试/静默终止」，不能覆盖或削弱 `EDGE-6` 对已确认履约的补完要求。音频最终失败时必须进入明确终态（如 fallback TTS / board-only / discarded），并留内部审计事件。

### D9 RULE-2、EDGE-3/5/8 红线
- `topic_offer` 与 `audio_play` 是两次**独立**的 1-bit 提议：各自等待 idle、各自过 cooldown/daily cap、设备端只收 `Yes/No/ABORT`，不保留会话态、不追问「有空吗」、不在同一屏串成多轮，因此不新增 RULE-2 例外。
- 长时间找不到第二次空闲窗口时，audio 候选按 TTL 衰减/过期（EDGE-3），不能堆积成待办债，也不能绕过 FR-G2 强推。
- 主题提取或音频生成失败/超时走降级、重试或静默终态（EDGE-5），但不向用户报错、不阻断其他候选。
- topic 候选生成前必须做跨文档/近似主题去重（EDGE-8），避免 N:M topic 拆分后给同一件事重复发卡。

---

## 取舍

- **取**：契约与真实意图一致；看板先成、音频按需，避免为未接受主题生成长音频；NotebookLM 音频调用集中在用户明确接受后的主题上，缓解 DEBT-2 / R2（限流、脆弱）。
- **取**：AC-2 与 FR-G2 仍按用户注意力成本计量，第二推不隐身、不扩容，真人小测能真实反映两阶段链路是否打扰。
- **取**：NotebookLM 作为主题提取主路径，换取 demo 第三方加工路径与 Provider 接缝收敛；音频不偏题由「已接受 topic/document 条件化输入」保证，不靠共享 NotebookLM 会话。internal LLM degraded fallback 只做可用性兜底，不承诺 N:M 等价或质量等价。
- **舍**：主题提取也走 NotebookLM 后，Track-A 对 NotebookLM 的 demo 依赖更重，并有意偏离 `design.md:466` 的旧防线；必须靠 Provider 抽象、唯一出境点、T0.16 spike、fallback、DEBT-2/R2 记账与 A 评审防止固化成不可替换核心。
- **舍**：每条主题最多两次主动推送，编排/状态/指标复杂度增加；需要 `push_kind` 分层、stage-2 No 特例、`AUDIO_REQUESTED` 耐久检查点与 topic-scoped 数据模型一起落地。
- **舍**：合并 `PARAM-DAILY_CAP(≤6)` 后，两阶段会把完整主题吞吐压到每日约 3 个主题；这是保持用户打扰预算不扩张的代价，可能导致候选等待或 TTL 过期。
- **守住红线**：`audio_play` 第二推必须是新的 idle+cooldown+daily-cap 决策窗口，不是 topic 卡后的自动续播、下一条、预问或链式延展，因此不新增 FR-D2/RULE-2/RULE-4 例外。

---

## 后果 / 波及面

- **K4（T0.4）**：`Candidate` 加 `kind`/`parent_id`；FSM 形状基本不变，但 `audio_play No/timeout` 是 stage-2 特例。
- **K6（T0.6）**：新增/细化事件（见 §8.4 清单）；`CONTENT_READY` 在 A 轨语义拆分；`AUDIO_REQUESTED` 必须成为可恢复事件/检查点。
- **K8（T0.8）**：`candidates` 表加 `kind`、`parent_id`；topic/board/audio 表按 topic-scoped 调整，并增加 document-topic N:M 映射；`topics.est_minutes` 不得保留为独立时长真相；新增/重建表必须继承 `purge_after`/Reaper、`origin='track_a'` 或等价 Track-A 归属护栏、tenant RLS，且事件/错误 payload 不得含原文、逐字稿或清洗正文。
- **FR-A4 / FR-A7**：保留 FR-A4 承载主题提议；新增 FR-A7 承载音频播放提议，避免 `FR-A4a/FR-A4b` 对 traceability 工具链隐形。
- **AC-2**：所有主动推送进入 `PUSH_SHOWN − ABORT` 主分母；`PUSH_SHOWN` 必须带 `track`/`push_kind`，供 MetricsEngine 分层展示。
- **FR-G2**：`PARAM-COOLDOWN` 与 `PARAM-DAILY_CAP` 合并计数，第二推不得绕过全局节流和每日上限。
- **EDGE-6**：`topic_offer YES` 后新增慢异步窗口，必须持久化 `AUDIO_REQUESTED` 并可恢复补完。
- **EDGE-8**：Track-A topic 层必须去重/近似合并，不能只依赖 Track-B 去重。
- **AC-1 / 看板必成**：主题提取新增 internal LLM degraded fallback，避免 NotebookLM 不可用时阻断 topic/board 产出；fallback 非等价边界要明示。
- **DEBT-2 / R2**：NotebookLM 风险范围扩大到 topic/audio 主路径；按需音频是缓解，但不能抵消主题提取上移带来的风险，应在 `debt/registry.py` 说明与 R2 缓解项中如实记录。
- **演示材料**：`presentations/demo-kickoff.html` 已先行反映音频侧两阶段修正（slide 7/8/12/25/27），但尚未覆盖本轮新增的 Track-A `AUDIO_REQUESTED` / EDGE-6 恢复红线；后续材料同步需纳入 §8.4 流程。

---

## §8.4 后续修订清单（A 批准后逐条执行）

本 ADR 记录 contract change 决策与落地路径；本轮由 Issue #38 承接 `management.md §8.4` 所需的 `contract-violation/task` 说明，并按小步 PR 顺序逐条执行以下修订。后续 PR/提交需引用 `Refs: #38` 以及 `T0.4/T0.6/T0.8` 或对应 WI Issue。

### 1) `spec.md`（冻结条款 · Contract Change）
- **FR-A3** 改：NotebookLM 提取主题；看板必成（提取后即生成，轻量，topic-scoped）；**音频在用户接受主题后按需生成**（NotebookLM，时长 `PARAM-AUDIO_LEN`）。同时写明 NotebookLM 失败时允许 internal LLM degraded fallback 产出 topic/board，但 fallback 不承诺跨文 N:M 等价。
- **FR-A4** 保留为「主题提议」：提取主题后，空闲以 1-bit 推送主题（OUT-1），不得要求用户主动查找/打开，无菜单。
- **新增 FR-A7** 为「播放提议」：音频就绪后，空闲以 1-bit 再推送播放提议。
- **FR-A5** 限定到「FR-A7 播放卡 YES 后」播放音频 + 看板，避免前向引用含混。
- **旅程一**（`spec.md:131-138`）按 D1 重写；删除「3 分钟」措辞。
- **§5.2 OUT-2** 注明 10–15 分钟（`PARAM-AUDIO_LEN`）。
- **FR-G2 / §11.2** 注明 topic/audio/Track-B 主动推送合并适用 cooldown 与 daily cap。
- **RULE-2** 注明两阶段是两次独立 1-bit 提议，不允许在设备端串成预问、多轮追问或会话态。
- **§11.2** 新增 `PARAM-AUDIO_LEN` 行。
- **§9 EDGE** 澄清：`topic_offer No/timeout` 按 EDGE-1 不重弹；`audio_play No/timeout` 表示「现在不听」、可受 TTL/节流有限重议；主题接受后必须以 `AUDIO_REQUESTED` 满足 EDGE-6 恢复补完；主题接受但音频生成失败 → 降级 TTS/看板-only 或明确终态（EDGE-5）；stage-2 长期无空闲 → audio 候选衰减过期（EDGE-3）；Track-A topic 必须去重（EDGE-8）。

### 2) `design.md`
- **§1.3**：`Candidate` 加 `kind`/`parent_id`；说明 A 轨顺序产出两候选、复用现状态集，但 `audio_play No/timeout` 是 stage-2 特例，会回到 READY 而非内容拒绝。
- **§1.4**：时序图重画为两阶段双推送（含「topic ACCEPTED → 持久化 `AUDIO_REQUESTED` → 触发 NotebookLM 按需生成 → `AUDIO_READY`」）。
- **§2.3 设备协议 `CARD_PUSH`**：增加 `push_kind`；将 `audio_ready` 改为仅对 `audio_play` 有意义的可选/条件字段，避免 topic_offer 卡被标成音频已就绪。
- **§3.2 Document 状态机**：`GENERATING(board ∥ audio)` 拆为「NotebookLM topic extraction + 看板 up-front 必成」+「音频延后到主题接受后」；新增子态 `TOPIC_READY → (await accept) → AUDIO_REQUESTED → AUDIO_GENERATING → AUDIO_READY`。
- **§3.3 NotebookLM Provider**：增加/调整 `TopicExtractionProvider`，NotebookLM 同时承载 topic extraction 与按需 audio generation 的默认 demo provider；主题提取必须有 internal LLM degraded fallback；两者仍经唯一出境点、SessionPool、隔离 worker 池、即用即删。
- **§3.4 数据模型**：topics/boards/audio_artifacts 以 topic 为主键或外键；增加 document-topic N:M 映射；candidates 以 `kind`/`parent_id` 串联两阶段；增加 `audio_requests` 或等价 job/outbox 表承载 `AUDIO_REQUESTED` 幂等重驱。新增/重建的全部 topic-scoped 表与 `audio_requests` 必须继承既有隐私底座：
  - **CON-5③ 限期清理**：每行带 `purge_after`，或纳入 Retention Reaper 的 `expires_at`/TTL 路径，与 `documents.purge_after`（design.md:452）、`data_assets.expires_at`（design.md:789）同口径；N:M 下须明确 topic 清理触发，避免一对多映射漏清，满足 T0.8 WI-2「限期清理字段存在」。
  - **RULE-6 物理隔离**：带 `origin='track_a' CHECK` 或等价 Track-A 归属护栏，对照 `documents.origin`（design.md:453），并纳入带基础 policy 的行级 tenant RLS（design.md:789），不能只 `ENABLE RLS` 而无 tenant policy，满足 T0.8 WI-3「物理隔离底座可验」。
  - **RULE-6 脱敏事件/错误字段**：`audio_requests.last_error`、terminal reason、审计事件 payload 只存结构化关联与 error code，error 仅内部，经 `obs.py` 脱敏单源；不得存 `clean_text`、原文片段、`raw_quote`、transcript 或音频内容。
  - **`est_minutes` 单源**：两阶段两卡（「想了解《主题》吗?」/「音频好了，现在听?」）均不显示时长，故删除 `topics.est_minutes`，`PARAM-AUDIO_LEN` 为预计时长唯一真相；若后续某卡需显示预计时长，则渲染期读取 `PARAM-AUDIO_LEN`，另设字段只可存生成后**实际**时长，不能回流为预计时长源。
- **§3.4 钩子文案**：保留并改写 `design.md:466` 的风险论证，不是简单删除；说明本 ADR 有意接受 NotebookLM topic 主路径，并以 fallback/Provider/DEBT 管理风险；同时删除「想花 3 分钟听 X」。
- **§3.5 可靠性降级阶梯**：从只覆盖音频扩展为覆盖主题层：`NotebookLM topic extraction → internal LLM degraded fallback → 静默重试/丢弃`；明确 fallback 不承诺 N:M 等价；音频层继续 `NotebookLM MP3 → tts_fallback → 看板-only/明确终态`。
- **§5.4 Orchestrator 决策环**：按 `candidate.kind` 分支，`topic_offer YES` 触发并持久化 `AUDIO_REQUESTED`，`audio_play YES` 触发播放履约；两者都走同一 idle/cooldown/daily-cap 单卡闸，满足 RULE-2。
- **§6.1 FeedbackSink**：按 `candidate.kind` 分支处理 No/timeout：`topic_offer` → DECLINED/不重弹；`audio_play` → READY/有限重议；ABORT 仍只调择时。
- **§11 事件**：新增 `TOPIC_READY`、`AUDIO_REQUESTED`（主题接受触发生成且可恢复）、`AUDIO_READY`；`PUSH_SHOWN` 带 `push_kind`；A 轨不再用单一 `CONTENT_READY` 同时代表看板+音频；新增事件 payload 仅含结构化 ID、状态、provider/error code 与参数版本，不含原文、clean_text、raw_quote、transcript 或音频内容。
- **§11.2 MetricsEngine / AC-2**：整体口径仍 `PUSH_SHOWN - ABORT`，并按 `track`/`push_kind ∈ {topic_offer, audio_play, track_b_seal}` 输出分层接受度。
- **§11.2 AC-1 度量行（design.md:869）**：把 EDGE-6「迟到补完」/旁路 `recovery_rate` 从仅 Track-B Notion outbox 口径扩展到 Track-A：`topic_offer YES → AUDIO_REQUESTED → AUDIO_READY/明确终态` 的崩溃恢复补完必须计入 AC-1，与 D8 可恢复检查点对齐。
- **§10.2**：登记 `PARAM-AUDIO_LEN`。
- **§4.5 / §5 / §10.2 候选 TTL 调和**：候选 TTL 不是新参数，默认复用既有内部参数 `PARAM-CAND_TTL`（design.md:555/845，默认 18h，「不跨业务日」）。`topic_offer` 保持现有不跨业务日语义；`audio_play` 为支持「现在不听」后的下一合适空闲窗口，允许跨一个业务日边界，但仍受 `created_at + PARAM-CAND_TTL` 绝对上限约束。若 A 后续不接受跨业务日再议，则必须把 D6/D9 改写为「audio 候选当日不取即过期」；任何新增可调界仍须登记并过 H3。
- **§B 附录映射表**：保留 `FR-A4` 到 `CARD_PUSH`/编排章节的映射，并新增 `FR-A7` 到 `CARD_PUSH`、两阶段编排、反馈分支、播放履约章节的映射；补 EDGE-6、EDGE-8、RULE-2 对应设计章节。

### 3) `config/params.py`
- 新增 `"AUDIO_LEN": Param("PARAM-AUDIO_LEN", 12, (5, 20), "behavior", "Track-A 音频生成")`（受 H3 校验）。
- 同步 `# PARAMSTORE-COMPLETE` 注释，从「7 个 PARAM」改为「8 个 PARAM」。
- 不新增假想候选 TTL 参数；按 §8.4 design 清单调和既有 `PARAM-CAND_TTL` 与 `audio_play` stage-2 生命周期。若最终新增 `audio_play` 专用 TTL/跨日界参数，必须同步新增 PARAM、登记 spec/design/traceability，并通过 H3。

### 4) `docs/traceability.yaml`
- 保留 `FR-A4` 行并改为主题提议语义；新增 `FR-A7` 行作为播放提议；新增 `PARAM-AUDIO_LEN` 行与 AC-2 `push_kind` 备注；补 EDGE-6、EDGE-8、RULE-2 的实现/测试占位说明；保持 `check_traceability.py --strict` 绿。
- 只有真实测试文件同步落地时，才把相关 `tests: [TODO]` 改成真实路径；不得在 ADR/矩阵里声称未实现测试已覆盖。

### 5) `docs/task.md`
- **M0**：`T0.4` 验收点加 `kind/parent_id` 和两类 No 语义；`T0.6` 加两阶段事件、`push_kind`、`AUDIO_REQUESTED`；`T0.8` 加 topic-scoped 表形、document-topic N:M、`audio_requests`/outbox 与 `candidates.kind/parent_id`，并补「新表/重建表带 `purge_after` 或纳入 Retention Reaper（WI-2）」与「新表带 `origin='track_a'` CHECK 或等价 Track-A 归属护栏 + 入 tenant RLS 基础 policy（WI-3）」验收点，使 CON-5③/RULE-6 底座对新持久层可验。
- **M2**：`T2.2` 主题提取 → NotebookLM N:M 主题候选 + internal LLM degraded fallback；`T2.3` 看板 up-front 必成；`T2.4` NotebookLM 音频 → topic 接受后按需触发；新增/改任务「主题接受 → 持久化生成请求 → audio 候选 → 二次推送」编排；`T2.8` 旅程一 e2e 按两阶段更新。

### 6) `debt/registry.py`
- 更新 `DEBT-2.summary`：
  - before: `依赖第三方加工能力 (NotebookLM 非官方/易 break)`
  - after: `依赖第三方加工能力 (NotebookLM 非官方/易 break；demo 主路径承担主题提取与音频生成)`
- 更新 `DEBT-2.surfaced_by`：
  - before: `Provider 抽象 + 三档降级阶梯; 每次调用 emit DEBT_EXERCISED; 失败计 EDGE-5 不报错`
  - after: `Provider 抽象 + 三档降级阶梯; 每次调用 emit DEBT_EXERCISED; 失败计 EDGE-5 不报错; 主题提取有 internal LLM degraded fallback; 按需音频仅对已接受主题调用; AUDIO_REQUESTED 满足 EDGE-6 重驱`
- 更新 `DEBT-2.repay_before`：
  - before: `替换为可控/自研音频生成, 保持 Provider 可热插拔`
  - after: `替换为可控/自研主题提取与音频生成, 保持 Provider 可热插拔`

### 7) `docs/management.md` / R2 风险表
- 更新 `R2`：
  - before: `NotebookLM 非官方、易 break、限流（DEBT-2） | 轨道 A 音频不稳 | Provider 抽象 + 三档降级阶梯（看板必成）；CI 用假 Provider；A 持有 Provider 接缝可热插拔（design §3.3/§3.5）`
  - after: `NotebookLM 非官方、易 break、限流（DEBT-2） | Track-A topic/audio 主路径不稳 | Provider 抽象 + topic degraded fallback + 音频三档降级；按需音频仅对已接受主题调用；AUDIO_REQUESTED 可重驱；CI 用 fake Provider；A 持有 Provider 接缝可热插拔（design §3.3/§3.5）`

### 8) 测试落点（随后续实现 PR 同步）
- `tests/contract/test_ac2_metrics.py`：钉死 `track`/`push_kind` 分层、所有主动推送进 `PUSH_SHOWN - ABORT`、ABORT 剔除。
- `tests/unit/test_candidate.py`：覆盖 `topic_offer No/timeout → DECLINED/不重弹` 与 `audio_play No/timeout → READY/有限重议`。
- `tests/unit/test_params.py`：覆盖 `PARAM-AUDIO_LEN` 默认值、区间与 behavior 参数身份。
- `tests/unit` 或 `tests/integration` 增加主题提取 fallback 覆盖：NotebookLM topic extraction 失败时 internal LLM degraded fallback 仍可产出 topic/board；若 fallback 退化为逐文档 topic，测试必须断言该降级被标记而非伪装成 N:M 等价。
- `tests/unit` 或 `tests/integration` 增加 EDGE-6 覆盖：`topic_offer YES` 后进程中断，恢复后 sweeper 仍能从 `AUDIO_REQUESTED` 重驱到 `AUDIO_READY` 或明确终态。
- `tests/unit` 增加 Track-A topic 级 EDGE-8 去重覆盖，避免近似主题重复推送。
- `tests/contract` 或红队脚本增加 RULE-2 覆盖：topic/audio 两张卡均为独立 1-bit，不产生预问、多轮追问或设备端会话态。
- `tests/integration` 增加新持久层隐私底座覆盖：`audio_requests` 与重建的 topic-scoped 表带 `purge_after` 或进入 Reaper 清理路径（CON-5③）；带 `origin='track_a'` 或等价护栏并纳入 tenant RLS 基础 policy，跨租户不可见（RULE-6）。
- `tests/contract` 增加 AC-1 度量覆盖：Track-A `AUDIO_REQUESTED` 崩溃恢复后补完计入 EDGE-6 迟到补完 / `recovery_rate`，而不只覆盖 Track-B outbox。
- `tests/contract` 或日志脱敏测试增加新事件/错误字段覆盖：`TOPIC_READY`、`AUDIO_REQUESTED`、`AUDIO_READY` payload 与 `audio_requests.last_error` 不含 `clean_text`、原文片段、`raw_quote`、transcript、音频内容等敏感 token，只保留结构化 ID 与 error code。

---

## 已拍板事项

1. **主题提取引擎**：交给 NotebookLM，作为 demo 默认 `TopicExtractionProvider`，但不得绕过 Provider 抽象和唯一出境点。
2. **主题提取可用性**：NotebookLM 是主路径，但必须有 internal LLM degraded fallback；看板 up-front 必成不以 NotebookLM 必然可用为前提；fallback 不承诺跨文 N:M 等价。
3. **`design.md:466` 风险接受**：保留旧论证的风险事实，但本 ADR 有意选择 NotebookLM topic 主路径，并通过 Provider/fallback/DEBT/R2/A 评审管理该风险。
4. **ID 命名**：保留 `FR-A4` 作为主题提议，新增 `FR-A7` 作为播放提议；不使用 `FR-A4a/FR-A4b`。
5. **AC-2 口径**：两次推送都计入主分母；以 `track`/`push_kind` 分层展示，主指标仍看整体 `composite_acceptance`。
6. **节流/日上限**：topic/audio/Track-B 主动推送合并计数，共用 `PARAM-COOLDOWN` 与 `PARAM-DAILY_CAP`。
7. **看板生成时机**：看板 up-front 必成，播放时呈现，不延后到 audio 阶段。
8. **stage-2 No**：`audio_play No/timeout` 表示「现在不听」，不等同 EDGE-1 的 topic 拒绝；可有限重议，但仍受全局节流、daily cap 与 TTL 控制。
9. **EDGE-6**：`topic_offer YES` 后的音频生成请求必须持久化并可恢复重驱，不能因慢异步生成窗口丢失已确认履约。

---

## 验收链重评（§8.4 要求）

- **AC-1 链路贯通（A 轨）**：判定标准更新为「NotebookLM 提取主题 → 推送① → 接受 → 持久化 `AUDIO_REQUESTED` → 按需生成 → 推送② → 播放」端到端至少一条贯通；主题提取 fallback 必须保证 NotebookLM 主题提取不可用时仍可产出 topic/board，并明确标记非等价降级；`T2.8` 走查脚本同步；AC-1 度量行（design.md:869）须同步把 EDGE-6 迟到补完 / `recovery_rate` 覆盖 Track-A `AUDIO_REQUESTED` 恢复，而非仅 Track-B outbox。
- **AC-2 接受度**：所有主动推送计入整体分母，并按 `track`/`push_kind` 分层；后续由 MetricsEngine contract test 钉死。
- **AC-4 屏纪律**：两次交互各自 time-to-dark ≤ `PARAM-TTD`，不变。
- **AC-3**：RULE 集不变；`topic_offer` 与 `audio_play` 是两次独立 1-bit 提议，设备端不预问、不追问、不保留会话态；`audio_play` 第二推必须是独立 idle+节流窗口，不是自动续播、自动下一条或链式延展，因此不新增 RULE-2/FR-D2/RULE-4 例外。
- **AC-5**：DEBT 集合不变，但 DEBT-2 的 surfaced_by/偿还说明要覆盖 NotebookLM 主题提取主路径、fallback 非等价边界、按需音频、`AUDIO_REQUESTED` 重驱与最终替换路径；新增/重建持久层必须继承 CON-5③ 留存清理、RULE-6 隔离与脱敏底座，不能因 D8 新表逃逸既有隐私约束。

> **当前 M0 落地边界**：PR #39 / `af18906` 及后续修正只冻结两阶段 contract 与 seam 形状。EDGE-6 worker+sweeper 重驱、AC-1 `recovery_rate` 面板、board 生成链路、真实 NotebookLM→internal LLM fallback 编排与 Track-A e2e 仍属 T0.8/T2.x 后续实现；绿 CI 不能解读为这些 runtime 行为已完成。

> 落地顺序：建 `contract-violation`/`task` Issue → A 批准 → 单独 PR 按本清单改 spec/design/task/traceability/params/registry/management（标 Contract Change，A 必审，跑 H1–H5 + `--strict` + 对应 tests）→ 本 ADR 状态改 Accepted。
