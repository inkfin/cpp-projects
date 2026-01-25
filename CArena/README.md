# CArena

栈式（bump/stack）memory arena

0. 先把“栈式 arena”的语义钉死

你要实现的典型语义通常是：
	1.	线性分配（bump）：只增不减，alloc 只移动指针。
	2.	LIFO 回滚（stack mark）：用 mark = current_offset 记录现场；之后可 reset_to(mark) 一次性回滚。
	3.	不支持任意 free：只支持回滚到 mark 或整体 reset。
	4.	对齐可控：支持 align（至少到 max_align_t，最好允许 caller 指定）。

你先决定：你的 arena 需要到什么程度？
	•	仅单块 buffer（固定容量）？
	•	允许自动扩容（链表/块数组）？
	•	线程安全要不要？（栈式一般做 TLS 或调用方保证单线程）

下面路线默认：单线程、可扩容（可选）、支持 mark/reset、支持对齐、可观测统计。

1. 设计最小数据结构（只存必要字段）

第一步你只实现“固定容量、单块 arena”，字段建议：
	•	uint8_t* base;
	•	size_t capacity;
	•	size_t offset;（当前 bump 指针相对 base 的偏移）
	•	（可选）size_t high_water;（统计峰值）
	•	（可选）uint32_t flags;（比如是否 owns memory）

关键问题：
	•	base 从哪里来？（用户传入 buffer，或 arena 内部 malloc）
	•	你是否允许 arena 管理内存生命周期？（决定是否需要 destroy）

你先实现两个构造模式：
	1.	arena_init_with_buffer(arena*, void* buf, size_t cap)
	2.	arena_init_malloc(arena*, size_t cap) + arena_destroy(arena*)

2. 先写对齐函数（这是 bump arena 的核心）

你需要一个“向上取整到 alignment”的工具：
	•	输入：size_t offset, size_t align
	•	输出：size_t aligned_offset
	•	约束：align 必须是 2 的幂（否则要么 assert，要么返回失败）

思路：
	•	aligned = (offset + (align - 1)) & ~(align - 1) 这一类位运算。
	•	但要小心溢出：offset + (align - 1) 可能溢出 size_t（一般 cap 不会那么大，但建议写成“先检查再加”）。

你实现完后立刻写 5 个断言用例：
	•	offset 已对齐
	•	offset 未对齐
	•	align=1
	•	align=8/16
	•	非 2 次幂 align 的行为（assert 或返回错误）

3. 实现最小 alloc：arena_alloc(arena*, size_t size, size_t align)

分配算法就是三步：
	1.	off2 = align_up(arena->offset, align)
	2.	end = off2 + size
	3.	若 end > capacity -> 失败（返回 NULL 或错误码）
	4.	arena->offset = end
	5.	返回 base + off2

决策点：
	•	失败策略：返回 NULL vs 返回错误码（更建议：void* 返回 + errno/bool 输出参数）
	•	size==0 你怎么处理？（常见做法：返回一个非 NULL 的“可唯一地址”很难保证；更简单：size==0 也走逻辑但不推进，或强制返回 NULL。建议：允许 size==0，返回对齐后的地址但不推进/推进 0，都可以，只要一致。）

此时你就可以做一个最小 demo：分配几个 struct，写入字段，验证地址单调递增且满足对齐。

4. 加入“栈语义”：mark / reset_to / reset

这是栈式 arena 的灵魂。

建议接口：
	•	typedef size_t ArenaMark;
	•	ArenaMark arena_mark(const Arena* a) -> 返回 a->offset
	•	void arena_reset_to(Arena* a, ArenaMark m) -> a->offset = m（需要边界检查）
	•	void arena_reset(Arena* a) -> reset_to(0)

决策点（很重要）：
	•	reset_to 是否允许 m 指向“中间非对齐位置”？
一般允许，因为 mark 只是 offset 快照；对齐由下一次 alloc 解决。
	•	debug 模式下，你可以在 reset 后把回滚区间写成 0xCD（便于发现 UAF），release 关闭。

5. 做一个“临时作用域”用法（不用宏也可以）

为了让调用方更不容易用错，你可以鼓励这种模式：
	•	进入函数：ArenaMark m = arena_mark(a);
	•	临时分配一堆东西
	•	退出前：arena_reset_to(a, m);

你也可以提供一个小 helper（仍然不替你写全代码，只给思路）：
	•	ArenaScope struct：保存 Arena* + mark
	•	arena_scope_begin(...) / arena_scope_end(...)

C 没有 RAII，但你可以用 goto cleanup 结构让错误路径也能统一回滚。

6. 可选：扩容（多块 chunk 链表），把 arena 变成“可增长栈”

如果你希望 “cap 不够就自动 grow”，你需要引入 chunk：
	•	每个 chunk：base/capacity/offset/prev
	•	arena 持有 current_chunk 指针
	•	alloc 先在 current 上试；不够就新建更大的 chunk（如：max(size+align, current_cap*2)）

关键语义问题（你必须提前决定）：
	1.	mark 如何表示？
需要包含：chunk指针 + chunk内offset。
所以 mark 不能再只是 size_t，而应是：
	•	struct { Chunk* c; size_t off; }
	2.	reset_to(mark)：
	•	回滚当前 chunk offset
	•	并释放 mark 之后新增的 chunk（如果 chunk 是 malloc 的）
	3.	是否允许“用户提供的外部 buffer chunk”？（混合所有权会复杂）

扩容是第二阶段，不建议一开始就上，否则你会同时调试对齐、mark、chunk 生命周期，错误很难定位。

7. 加“接口层”让它更像库：统计、错误、调试钩子

实用性提升通常来自这些小功能：
	•	arena_remaining(a) / arena_used(a) / arena_capacity(a)
	•	arena_high_water(a)（峰值）
	•	失败时记录最后一次失败请求：size/align（便于日志）
	•	自定义 allocator 回调（让你以后能接你自己的 tl/对象池风格接口）

8. 写单元测试与“典型用例 demo”（你写代码时的验收清单）

你每写完一步就跑一组测试，避免后面全炸：
	1.	对齐：返回指针 % align == 0
	2.	容量边界：刚好填满、超 1 字节失败
	3.	mark/reset：
	•	分配 A/B/C，mark 在 B 后，reset_to(mark) 后再分配 D，D 的地址应等于 C 起始（或按对齐规则一致）
	4.	嵌套 mark：mark1/mark2 回滚顺序正确
	5.	（可选 grow）跨 chunk 的 mark/reset 正确释放 chunk

9. 你现在就可以开始写的“最小接口清单”

你先只写这些（固定容量版）：
	•	init/destroy（二选一：外部 buffer 或 malloc）
	•	align_up 工具
	•	alloc(size, align)
	•	mark/reset/reset_to
	•	used/remaining

写完就能在游戏里做：
	•	每帧 reset 的临时分配（比如构建 projectile 列表、碰撞临时数组）
	•	函数内临时 scratch（mark-scope）

