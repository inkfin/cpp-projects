好的，给你一份 Lua 栈与索引（index）速通 + 逐条说明。我按“心智模型 → 索引规则 → 常用函数分组（逐条解释）→ 典型例子 → 易错点”来讲。所有说明以 Lua 5.4 C API 为基准（5.1/5.3 基本一致，个别函数名略有差别）。

⸻

心智模型（一定先建立）
	•	Lua 有一条栈（stack），专给 C API 交互用。你向里 push 值、从中 取/改 值。
	•	**索引（index）**是访问栈位的方式：
	•	正索引：从底部开始计数，1 是栈底，第 n 个就是 n。
	•	负索引：从顶部向下计数，-1 是栈顶，-2 是栈顶下面的那个，依次类推。
	•	伪索引（pseudo-index）：不是栈上的实位，而是特殊入口：
	•	LUA_REGISTRYINDEX（注册表）
	•	lua_upvalueindex(i)（当前 C 闭包的第 i 个 upvalue）
	•	push/pop 会改变栈高，**负索引始终“贴着栈顶”**很方便；正索引在 push/pop 后“相对位置可能变”，需要用 lua_absindex 固化。

⸻

索引规则（必须非常熟）
	•	lua_gettop(L) → 返回当前栈顶索引（也是元素个数）。空栈返回 0。
	•	任意时刻，合法的栈索引范围是 1..lua_gettop(L)；负索引合法范围是 -lua_gettop(L)..-1。
	•	lua_absindex(L, idx) → 把 idx（正/负/伪索引）转换成正的稳定索引（伪索引保持不变）。
用它可“锁定”一个元素位置，避免后续 push/pop 造成引用错位。

⸻

A. 栈查询与类型检查（读栈不改栈）
	•	int lua_gettop(L)：返回栈中元素个数（= 栈顶索引）。
	•	int lua_type(L, idx)：返回 idx 处类型枚举（LUA_TNIL/LUA_TNUMBER/…）。
简便：const char* lua_typename(L, t)
	•	int lua_is.../lua_to... 家族（只读，不会弹栈）：
	•	int lua_isnil(L, idx)、int lua_isnumber(L, idx)、int lua_isstring(L, idx)…
	•	lua_Integer lua_tointeger(L, idx)、lua_Number lua_tonumber(L, idx)
	•	const char* lua_tostring(L, idx)（底层等价 lua_tolstring(L, idx, NULL)；返回内部缓冲指针，有效期受栈变动影响）
	•	void* lua_touserdata(L, idx)、void* lua_touserdata（full/light 共用此取指针接口）
	•	size_t lua_rawlen(L, idx)（字符串/表/用户数据长度）
	•	int lua_iscfunction(L, idx)：是否 C 函数。

luaL_check* 系列（来自 lauxlib）会检查类型并在错误时抛 Lua 异常：
	•	luaL_checkinteger(L, idx)、luaL_checknumber(L, idx)、luaL_checkudata(L, idx, mtname)…

⸻

B. 纯“栈形状”操作（改栈形状，不看内容）
	•	void lua_settop(L, idx)：重设栈顶到 idx。
	•	idx < 0：等价于 lua_gettop(L) + idx + 1。
	•	idx > top：会在顶部补 nil。
	•	常见宏：lua_pop(L, n) == lua_settop(L, -(n) - 1).
	•	void lua_pushvalue(L, idx)：把 idx 处的值复制一份 push 到栈顶。
	•	void lua_remove(L, idx)：删除 idx 处的元素，上面的元素整体下移。
	•	void lua_insert(L, idx)：把栈顶元素 插入 到位置 idx（原来 idx..top-1 的元素整体上移）。
	•	void lua_replace(L, idx)：用栈顶元素替换 idx 处的值，并把栈顶弹掉。
	•	void lua_copy(L, from, to)：把 from 处的值复制覆盖到 to 处（不改栈高）。
	•	void lua_rotate(L, idx, n)：以 idx..top 为区间旋转，n>0 向上旋，n<0 向下旋。
	•	int lua_checkstack(L, sz)：确保额外 sz 个空位可用；必要时增长 C 栈段。

索引移动易错点：
remove/insert/replace/rotate 会改栈中元素的位置。想“钉死”某个元素，先用 lua_absindex.

⸻

C. push 一些值到栈顶（只增高）
	•	lua_pushnil(L)
	•	lua_pushinteger(L, i) / lua_pushnumber(L, n) / lua_pushboolean(L, b)
	•	lua_pushlstring(L, s, len) / lua_pushstring(L, s)（遇到 0 截断） / lua_pushfstring(L, fmt, ...)
	•	lua_pushlightuserdata(L, p)（一个裸指针）
	•	lua_pushcclosure(L, f, nup)：把 C 函数 f 与栈顶的 nup 个 upvalue 绑定，创建闭包并 push。
	•	用法：先把 nup 个值先后 push，再调用本函数；它会从栈顶“吃掉”这 nup 个值，封进函数里。

⸻

D. 表/元表/字段存取（会 pop/push，重要）

（以下 idx 都是表所在位置，可正/负/伪索引）
	•	取字段：
	•	void lua_gettable(L, idx)：从 idx 表里取 table[key]，其中 key 必须先 push 在栈顶；取到的值push 到新栈顶并弹出 key。
	•	void lua_getfield(L, idx, "k")：等价 push "k"; gettable。
	•	void lua_geti(L, idx, i)：等价 push i; gettable（数组索引）。
	•	void lua_rawget(L, idx) / lua_rawgeti/rawgetp：不触发元方法的 raw 取。
	•	写字段：
	•	void lua_settable(L, idx)：栈顶必须依次是 value、key，调用后两者被弹出并执行 table[key]=value。
	•	void lua_setfield(L, idx, "k")：等价 push "k"; settable（栈顶必须已有 value）。
	•	void lua_seti(L, idx, i)：等价 push i; settable。
	•	void lua_rawset(L, idx) / lua_rawseti/rawsetp：raw 写，不触发 __newindex。
	•	创建表：
	•	void lua_createtable(L, narr, nrec)：预留数组/哈希位（优化装载），并 push 新表。
	•	void lua_newtable(L)：等价 createtable(0, 0)。
	•	元表：
	•	int lua_getmetatable(L, idx)：取 idx 值的元表，存在则 push 并返回 1，否则返回 0。
	•	int lua_setmetatable(L, idx)：把栈顶表设为 idx 值的元表，并把栈顶弹出。

记忆法：
	•	“get” 系列读表：最终会 push 一个值。
	•	“set” 系列写表：需要你把 value（和可能的 key）预先 push 好，调用后会 弹掉它们。

⸻

E. 调用 & 错误处理
	•	把被调函数和参数按顺序 push 到栈顶（函数在底、参数在上），然后：
	•	int lua_pcall(L, nargs, nresults, msgh)：保护式调用（不会直接 longjump 出 C），
	•	nargs 参数个数，按压栈顺序读取；
	•	nresults 结果个数（LUA_MULTRET 表示尽可能多）；
	•	msgh 为错误处理函数索引（常见写法：debug.traceback），为 0 表示无处理。
	•	返回 0 表示成功，否则是错误码。成功时返回值会覆盖掉函数位置。
	•	辅助把 debug.traceback 放在栈上当 msgh 的常见模板：

lua_getglobal(L, "debug");
lua_getfield(L, -1, "traceback");
lua_remove(L, -2);           // 只留 traceback 在栈顶
int msgh = lua_gettop(L);    // 记录其索引（负索引更安全：-1）
// ... push 函数与参数
int rc = lua_pcall(L, nargs, nret, msgh);
lua_pop(L, 1);               // 弹出 msgh

	•	这里的 msgh 是栈上的函数位置索引；lua_pcall 需要的是“在当前栈里的那个函数的索引”，不是 C 函数指针。

⸻

F. 用户数据与 uservalue（与索引相关的新增位点）
	•	Full userdata 创建：void* p = lua_newuserdata(L, sz) → push 一个 userdata 值。
	•	int lua_setuservalue(L, idx) / lua_getuservalue(L, idx)（5.3 单槽）
void lua_setiuservalue(L, idx, n) / int lua_getiuservalue(L, idx, n)（5.4 多槽）
	•	set*：把栈顶值设为 idx 的 uservalue（或第 n 槽），并弹出栈顶。
	•	get*：把 idx 的 uservalue（或第 n 槽）push 到栈顶。

⸻

G. 注册表 & upvalue（伪索引）
	•	注册表：LUA_REGISTRYINDEX（伪索引）
	•	你可以在注册表里存全局 C 状态（通常用唯一 lightuserdata 当 key）。
	•	访问注册表用表 API：

// reg[key] = value
lua_pushlightuserdata(L, key);
lua_pushvalue(L, valueidx);
lua_settable(L, LUA_REGISTRYINDEX);

// value = reg[key]
lua_pushlightuserdata(L, key);
lua_gettable(L, LUA_REGISTRYINDEX);


	•	upvalue：lua_upvalueindex(i)（伪索引）
	•	当前 C 闭包的第 i 个 upvalue 就在这个“索引位置”。
	•	lua_pushcclosure(L, f, nup) 绑定 upvalues 后，在 f 里可用 lua_tovalue(L, lua_upvalueindex(i)) 取回。

⸻

H. 典型“索引变化”例子（看懂一次就不晕）

假设栈初始是空：

lua_pushinteger(L, 10);        // 栈: [10]           (top=1)
lua_pushinteger(L, 20);        // 栈: [10,20]        (top=2)
lua_pushinteger(L, 30);        // 栈: [10,20,30]     (top=3)

此时：
	•	1 指向 10，2 指向 20，3/-1 指向 30，-2 指向 20，-3 指向 10。

lua_insert(L, 2);              // 把栈顶插到位置 2
// 过程：取出 30 插到 2，原 [20] 与其后整体上移
// 栈变成: [10,30,20] (top=3)

lua_remove(L, 1);              // 删除位置 1 的元素（10）
// 栈: [30,20] (top=2)

lua_pushvalue(L, 1);           // 复制 idx=1 的值（30）到栈顶
// 栈: [30,20,30] (top=3)

lua_replace(L, 2);             // 用栈顶替换 idx=2，然后弹顶
// 栈: [30,30] (top=2)

如果你在 lua_insert 之前用 int i = lua_absindex(L, 2); 固化了“第二个元素”的位置，那么后续即使旋转/插入，i 仍指向当初的那个元素（除非你把它删掉）。这就是 lua_absindex 的意义。

⸻

I. 高频易错点（必看）
	1.	正索引“飘移”：push/pop/insert/remove 会改变“第 2 个”的含义。
→ 解决：需要长期引用时，马上 lua_absindex。
	2.	lua_tostring 返回指针易失效：后续任何会改变该值的操作（甚至 pushfstring）都可能让指针失效。
→ 解决：立刻 strdup 或 lua_pushlstring 再存一份。
	3.	settable/setfield 的入栈顺序：先 push value；对 settable 还要再 push key，顺序是（下是 key，上是 value）？
	•	记牢：settable 需要栈顶形如 [... table, key, value(top)] 吗？不是。
	•	正确：settable(idx) 要求顶部是 value，其下是 key（从顶向下依次 value, key），然后消耗它们。
但你通常这样写更直观：

lua_pushvalue(L, idx);      // push table 的拷贝到栈顶
lua_pushstring(L, "k");     // key
lua_pushinteger(L, 123);    // value
lua_settable(L, -3);        // 针对这个“栈顶-3”的 table 写入
lua_pop(L, 1);              // 弹掉那份 table 拷贝

或者更简：lua_setfield(L, idx, "k")（只要栈顶已有 value）。

	4.	lua_pcall 的 msgh：它要的是“栈上的错误处理函数的位置索引”，而不是 C 函数指针或全局名字。
→ 先把处理函数 push 到栈上（常用 debug.traceback），记住其索引（用 -1 最方便），再传给 lua_pcall。
	5.	伪索引不能当作普通整数处理：LUA_REGISTRYINDEX、lua_upvalueindex(i) 可能是负的大常数；用 lua_absindex 时会“原样返回”。

⸻

结语
	•	index 的本质：对“当前栈形状”的位置引用；负索引贴着栈顶很稳，正索引配合 lua_absindex 才稳。
	•	操作分三类记忆：改形状（insert/remove/replace/rotate）、读写表（get*/set*）、函数调用（pcall）。
	•	多写几个 小实验（如上 H 段）自己跑一遍，所有疑惑都会消失。

如果你愿意，我可以给你一个可编译的小程序：逐行打印每步栈内容（元素类型与值）、演示所有“改形状”API的效果，跑一遍就彻底通了。
