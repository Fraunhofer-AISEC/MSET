# Temporal Bug Types MLIR 化完整修改计划

> 原则：最小修改代码、不考虑 MLIR SSA 特性、以保留 C 语义为主、仅字符串替换。
> 本文档整合自原 `PROJECT_UNDERSTANDING.md` 与 `PHASE5_USEAFTERSTAR_PLAN.md`。

---
**所有涉及全局变量的 access 调用前，都必须手动插入 `memref.get_global` + `memref.load` 这两行，得到局部 SSA value 后再传入 `generate` / `generate_split_const_vars`**。


## 文件 1：`src/generator/primitives/bug_types/temporal/double_free.cpp`

### 原实现方法
直接嵌入 C 代码字符串：
- `#ifndef __GLIBC__` / `_exit(PRECONDITIONS_FAILED_VALUE)` —— glibc 预检查
- `char *pointer_to_double_free; char *pointer_to_use;` —— 指针声明
- `malloc(10)` / `malloc(8)` / `free(...)` —— 堆分配与释放
- `pointer_to_double_free[sizeof(void *)] = 0;` —— use-after-free 写操作
- `char *tmp, *tmp2, *tmp3;` —— 辅助指针（仅 variant_without_use_after_free）
- `_use(tmp2); _use(tmp3);` —— 防优化标记
- `pointer_to_use = (char *)malloc(8);` → access via `pointer_to_use` → `exit(TEST_CASE_SUCCESSFUL_VALUE)`

### 现修改计划
1. **移除** `#ifndef __GLIBC__` 判断（MLIR 不依赖 glibc）。
2. **保留** `_use(tmp2)` / `_use(tmp3)` / `_use(pointer_to_use)` 等调用，翻译为 `func.call @use(%ptr) : (memref<8xi8>) -> ()`。需要在 globals 区域补充定义：
   ```mlir
     func.func @use(%arg0: memref<8xi8>) -> memref<8xi8> { return %arg0 : memref<8xi8> }");
  
   ```
3. `tmp`/`tmp2`/`tmp3` 的 `malloc(8)` 保留，翻译为 `memref.alloc() : memref<8xi8>`（variant_without_use_after_free 中的干扰分配）。
4. `pointer_to_double_free = (char *)malloc(10);` → `memref.alloc() : memref<10xi8>`（核心对象）。
5. `pointer_to_use = (char *)malloc(8);` → `%pointer_to_use = memref.alloc() : memref<8xi8>`（新分配对象），随后 `access_location->generate(action, "pointer_to_use", 8)` 继续复用已有接口。
6. `free(pointer_to_double_free)` → `memref.dealloc %pointer_to_double_free : memref<10xi8>`。
7. `pointer_to_double_free[sizeof(void *)] = 0;` → `memref.store %c0_i8, %pointer_to_double_free[%c8] : memref<10xi8>`（variant_with_use_after_free 保留）。
8. `exit(TEST_CASE_SUCCESSFUL_VALUE)` → `func.return %test_success : i32`。

### 原因
- `HeapRegion` 已 MLIR 化，但 `DoubleFree` 额外硬编码的 `malloc/free` 字符串与其不兼容，必须逐条替换。
- `_use()` 在 C 中用于防止编译器优化掉未使用的变量；MLIR 前端虽无此问题，但保留 `_use` 调用可维持测试用例的原始语义结构，翻译为 `func.call @use` 即可。
- 核心语义必须保留：alloc → dealloc →（可选 dealloc 后 store）→ double-dealloc → **alloc 新对象 `pointer_to_use` → access `pointer_to_use`** → return success。

---

## 文件 2：`src/generator/primitives/bug_types/temporal/misuse_of_free.cpp`

### 原实现方法
直接嵌入 C 代码字符串：
- `target[8] = 0x40;` / `target[13*8] = 0x40;` —— 向 target 写 magic value 伪造 chunk
- `#ifndef __GLIBC__` / `_exit(PRECONDITIONS_FAILED_VALUE)` —— glibc 预检查
- `unsigned long *crafted_ptr; crafted_ptr = &(target[2*8]); free(crafted_ptr);` —— 取子区域地址并释放
- `char* heap_obj; heap_obj = (char *)malloc(8);` —— 重新分配堆对象
- `if ( &<target_name>[2*8] != heap_obj ) _exit(PRECONDITIONS_FAILED_VALUE);` —— 地址匹配检查（UsedMemory）
- `exit(TEST_CASE_SUCCESSFUL_VALUE)` —— 成功退出

### 现修改计划
1. **移除** `#ifndef __GLIBC__` 判断。
2. `target[8] = 0x40;` → `memref.store %c0x40_i8, %target[%c8] : memref<160xi8>`（magic value 保留为 `memref.store`，注释说明无 chunk 语义）。
3. `crafted_ptr = &(target[2*8]); free(crafted_ptr);` → `memref.subview` 取子区域 + `memref.dealloc`：
   ```mlir
   %crafted = memref.subview %target[16][8][1]
             : memref<160xi8> to memref<8xi8, strided<[1], offset: 16>>
   memref.dealloc %crafted : memref<8xi8, strided<[1], offset: 16>>
   ```
   （已验证 `memref.dealloc %subview` verifier 通过。）
4. `heap_obj = (char *)malloc(8);` → `%heap_obj = memref.alloc() : memref<8xi8>`。
5. UsedMemory 的地址匹配检查 `if (&target[2*8] != heap_obj)` → `memref.extract_aligned_pointer_as_index` 比较（若需要保留）
6. `exit(...)` → `func.return %test_success : i32` / `func.return %precond_fail : i32`。

### 原因
- 核心 bug 语义是"释放子区域后 access"，`memref.subview` + `memref.dealloc` 能保留这一语义。
- Magic values（`0x20`/`0x40`/`0x60`）在 MLIR 中无 chunk 语义，保留为 `memref.store` 仅维持代码结构。
- `accepts(region)` 原返回 `true`，但实际代码只处理 heap，MLIR 化时暂不修复，保持原接口兼容。

---

## 文件 3：`src/generator/primitives/bug_types/temporal/use_after_star.cpp`

### 原实现方法
直接嵌入 C 代码字符串，包含多个子函数：
- `_generate_unused_mem_heap`：全局变量 `char *target_address;` → alloc → init → free → 在 `deallocation_pos` 保存 `&target[0]` → access via `target_address`
- `_generate_unused_mem_stack`：全局变量 `char *target_address;` → alloca in `f()` → `target_address = &target[0];` → access in `main()` via `target_address`
- `_generate_reused_mem_heap`：alloc → `target_address = &target[0]` → free → `realloc` 循环 → `GET_ADDR_BITS` 地址比较 → access
- `_generate_reused_mem_stack`：跨 `f()`/`other_f()`，含 simple/repeated/array/repeated+array 4 个变体，使用 `do { other_f(); } while(counter++ < N)` 重试循环
- `_exit(42)`/`_exit(43)`、`volatile`、`_use(ptr)`、全局数组 `char *target_addresses[16]`

### 现修改计划（已确认/更新）

#### 关键技术确认
1. `memref.global @target_ptr : memref<1xmemref<8xi8>>` 初始化语法合法（省略初始值作为外部全局）。
2. 保存 target 地址+ptr 的代码插入在 `dealloc` **之后**（对应 C 语义 `free(target); target_address = &target[0];`）。
3. `_use(reallocated)` 保留，翻译为 `func.call @use(%reallocated) : (memref<8xi8>) -> ()`，同 `double_free` 在 globals 区域补充 `@use` 函数定义。通用替换

#### 全局变量替换（双全局方案）
- `@target_address : memref<1xindex>` —— 存 `extract_aligned_pointer_as_index` 的地址值，用于**地址比较**
- `@target_ptr : memref<1xmemref<8xi8>>` —— 存 memref SSA value，用于**跨函数 access**
- 数组变体对应增加 `@target_addresses : memref<16xindex>` 和 `@target_arr : memref<1xmemref<16x8xi8>>`（存整个 2D memref，用于 access）

#### 地址比较
`GET_ADDR_BITS(ptr)` → `memref.extract_aligned_pointer_as_index %ptr : memref<8xi8> -> index`，然后用 `arith.cmpi eq` 比较。

#### 各函数修改

**`_generate_unused_mem_heap`**：
- 全局声明改为 `memref.global @target_address / @target_ptr`
- 在 `deallocation_pos` 之后插入：提取地址 → `memref.store` 到 `@target_address`；`memref.store %target` 到 `@target_ptr`
- access 前从 `@target_ptr` `memref.load` 出 `%saved_ptr`，再传给 `access_location->generate(action, "saved_ptr", 8)`
- `func.return %test_success : i32`

**`_generate_unused_mem_stack`**：
- 同理替换全局变量，保留 `f()` / `main()` 跨函数结构
- `generate_split_const_vars()` 接口不变，传入 `"saved_ptr"` 作为 `access_var_name`

**`_generate_reused_mem_heap`（simple + repeated 2 变体）**：
- **Simple (variant 0)**：alloc target → 存地址+ptr 到全局 → dealloc → 单次 alloc `%reallocated` → 提取地址 → `arith.cmpi eq` → `scf.if %eq` 中 access saved ptr → `func.return %test_success`
- **Repeated (variant 1)**：
  1. `heap_memory_region->generate(deallocation_pos, ..., "reallocated", ...)` 已生成 `%reallocated_out = memref.alloc() : memref<8xi8>`
  2. **循环前**：调用 `generate_deallocation("reallocated", ...)` dealloc 掉这个预分配的 `%reallocated_out`
  3. **scf.while 循环**（iter_args: `%counter`, `%matched`）：
     - `before` 区域：检查 `%counter < MAX`，condition 传 `%matched, %counter`
     - `do` 区域：
       - 调用 `generate_reallocation("reallocated", ...)` 生成 `%reallocated = memref.alloc() : memref<8xi8>` + init loop
       - `extract_aligned_pointer_as_index %reallocated` 提取地址
       - `arith.cmpi eq` 比较 target_addr 和 realloc_addr
       - `arith.ori` 更新 matched
       - 调用 `generate_deallocation("reallocated", ...)` dealloc 当前轮次的 `%reallocated`
       - `scf.yield %next_matched, %next_counter`
  4. **循环后**：load saved_ptr → access → `func.return %test_success`

**`_generate_reused_mem_stack`（4 变体）**：

**Simple (variant 0)**：
1. **全局声明**：`memref.global @target_address : memref<1xindex>` + `memref.global @target_ptr : memref<1xmemref<8xi8>>` + `func.func private @exit(%arg0: i32)`
2. **`f()` 中**：`generate("target", 8, true)` → alloca target → init → `extract_aligned_pointer_as_index` → store 地址到 `@target_address` → store `%target` 到 `@target_ptr`
3. **`other_f()` 中**：`generate_in_other_f("reallocated", 8, false)` → alloca reallocated → `extract_aligned_pointer_as_index` → load `@target_address` → `arith.cmpi eq`
4. **`scf.if %eq` then 区域**：load `@target_ptr` → `%saved_ptr` → `access_location->generate_split_const_vars(action, "saved_ptr", 8)` → `func.call @exit(%test_success)` → `scf.yield`
5. **`scf.if` 之后**：`func.call @exit(%precond_fail)`（不匹配直接退出）

**Repeated (variant 1)**：
1. **新增全局**：`memref.global @last_address : memref<1xindex>`
2. **`f()` 中**：同 Simple，保存 target 地址+ptr
3. **`main()` 中 `scf.while` 循环**（iter_args: `%counter`）：
   ```mlir
   %results = scf.while (%counter = %c0_main) : (index) -> index {
     %lt = arith.cmpi slt, %counter, %cMAX_main : index
     scf.condition(%lt) %counter : index
   } do {
   ^bb0(%counter_iter : index):
     %_ = func.call @other_f() : () -> i32
     %next_counter = arith.addi %counter_iter, %c1_main : index
     scf.yield %next_counter : index
   }
   func.call @exit(%precond_fail) : (i32) -> ()
   ```
   通过 `add_at(get_other_f_call_pos(), ...)` 在 `func.call @other_f()` 前后插入循环结构。
4. **`other_f()` 中**：
   - load `@last_address` → cmp eq `%realloc_addr` → `scf.if` → `func.call @exit(%precond_fail)`（重复无帮助）
   - store `%realloc_addr` 到 `@last_address`
   - load `@target_address` → cmp eq → `scf.if` → access → `func.call @exit(%test_success)`
   - 不匹配则正常 return（`return %c0_i32`），让 `main()` 循环继续

**Array Simple (variant 2)**：
1. **全局声明**：`@target_addresses : memref<16xindex>` + `@target_arr : memref<1xmemref<16x8xi8>>`
2. **`f()` 中**：`generate_array("target", 8, 16, true)` → alloca `memref<16x8xi8>` → init → store `%target` 到 `@target_arr` → `scf.for` 循环16次，每次计算 `base_addr + i*8`，store 到 `@target_addresses`
3. **`other_f()` 中**：
   - `scf.for iter_args(%found = %false, %idx = %c0)` 遍历 `@target_addresses`，`arith.cmpi eq` + `arith.ori` + `arith.select` 找匹配索引
   - `scf.if %found`：
     - load `@target_arr` → `%target_loaded`
     - **`generate_split_const_vars(action, "target_loaded", 8, 16, "idx")`**：接口内部直接生成 2D 索引访问代码，如 `memref.load %target_loaded[%idx, %i] : memref<16x8xi8>`，不再需要 `reinterpret_cast` + `subview`
     - `func.call @exit(%test_success)`
   - `else`：`func.call @exit(%precond_fail)`

**Array Repeated (variant 3)**：
- 复用 Array Simple 的 `f()` 保存逻辑 + Repeated 的 `main()` `scf.while` 循环
- `other_f()` 中保留 `last_address` 检查 + 16 地址遍历匹配逻辑
- 未匹配时正常 return，让 `main()` 循环继续


#### 通用替换
- `_exit(42)` / `_exit(43)` → **不再使用 `func.return`**，改为调用外部函数：
  ```mlir
  func.func private @exit(%arg0: i32)
  // ...
  func.call @exit(%test_success) : (i32) -> ()
  func.call @exit(%precond_fail) : (i32) -> ()
  ```
  原因：`func.return` 不能出现在 `scf.if` 等区域内部，而 `_exit` 是进程终止语义，用 `func.call @exit` 模拟最直接。
- `volatile` → **移除**
- `%c0 = arith.constant 0 : index`：`f()` 中的常量无法在 `other_f()` 中复用，每个 `other_f()` 需要自行定义。
- **保留默认 `return %c0_i32`**：`other_f()` 和 `main()` 模板末尾的默认 return 作为语法 terminator 保留，前面插入 `func.call @exit` 后语义上进程已终止， verifier 不会报错。

#### Validation 版本
- FreedMemory：access 发生在 dealloc 之前（heap）或不 dealloc（stack）
- UsedMemory：不进行地址检查，直接 access

**`_generate_reused_mem_stack_validation`（4 变体）**：

**Simple Validation**：
- 不复用 `other_f()`，直接在 `f()` 内完成：alloca target → 保存到全局 → `generate("reallocated", 8, false)` 在 `f()` 中 alloca reallocated → load `@target_ptr` → access → `func.call @exit(%test_success)`

**Repeated Validation**：
- `f()` 中：alloca target → 保存全局 → alloca reallocated → 设置 `last_address` → access → `return %c0_i32`（正常返回）
- `main()` 中：`scf.while` 循环调用 `f()`（同非 validation 的 Repeated `main()` 循环结构）
- 循环结束后：`func.call @exit(%test_success)`
- 保留 `last_address` 检查，若相同则 `func.call @exit(%precond_fail)`

**Array Simple Validation**：
- `f()` 中：`generate_array` → 循环保存16个地址 + store `%target` 到 `@target_arr`（同非 validation）→ `generate("reallocated", 8, false)` 在 `f()` 中 alloca reallocated
- 保留 `scf.for` 遍历16个 target 地址的循环结构，未匹配时固定 `%idx = %c0`
- load `@target_arr` → `%target_loaded` → `generate_split_const_vars(action, "target_loaded", 8, 16, "idx")` → access → `func.call @exit(%test_success)`

**Array Repeated Validation**：
- `f()` 中：同 Array Simple Validation，但正常返回而非 exit
- `main()` 中：`scf.while` 循环调用 `f()`，循环结束后 `func.call @exit(%test_success)`
- 保留 `last_address` 检查



### 原因
- `UseAfterStar` 是最复杂的 temporal bug type，涉及跨函数指针保存、地址比较、重试循环、数组变体。
- MLIR 中 memref 是 SSA value，不能直接用 C 指针语义跨函数传递。双全局变量方案是**最小侵入**的解法：一个存 `index` 地址用于比较，一个存 `memref` 用于 access，两者互不转换。
- **Repeated heap 变体必须保留循环内 dealloc/alloc**：原 C 语义就是反复 free/malloc 直到地址匹配。通过 `scf.while` 的 iter_args 传递 `%reallocated_iter`，在 do 区域直接拼接 `generate_deallocation("reallocated_iter", ...)` 和 `generate_reallocation("reallocated", ...)` 的返回字符串，再 yield 新分配的 `%reallocated`，即可在合法 MLIR 中精确还原该语义，无需修改 Region 接口。
- `AccessLocation` 接口新增 `array_size` 和 `index_var` 默认参数（`array_size=0` 时行为完全不变），使 `generate` / `generate_split_const_vars` 原生支持 2D memref 数组元素访问，消除 `use_after_star` array 变体的 `reinterpret_cast` workaround。

---

## 修改文件清单

| 文件 | 修改类型 | 说明 |
|------|----------|------|
| `src/generator/primitives/bug_types/temporal/double_free.cpp` | 字符串替换 | C malloc/free → MLIR alloc/dealloc；`_use` 改为 `func.call @use`；保留 tmp 辅助分配 |
| `src/generator/primitives/bug_types/temporal/misuse_of_free.cpp` | 字符串替换 | 引入 memref.subview + memref.dealloc；magic value 改为 memref.store |
| `src/generator/primitives/bug_types/temporal/use_after_star.cpp` | 字符串替换 | 双全局变量、extract_aligned_pointer_as_index、`scf.while` + iter_args 保留循环内 realloc |
| `src/generator/primitives/access_types/access_location.h` | 接口修改 | `generate` / `generate_split_const_vars` / `generate_split_aux_vars` 新增 `array_size` 和 `index_var` 默认参数 |
| `src/generator/primitives/access_types/direct_location.h/.cpp` | 实现修改 | 实现新增参数，支持 2D memref 数组元素访问 |
| `src/generator/primitives/access_types/stdlib_location.h/.cpp` | 实现修改 | 实现新增参数，支持 2D memref 数组元素访问 |
| `src/generator/primitives/regions/heap_region.h` | 接口修改 | `generate_deallocation` 增加 `size_t size` 参数 |
| `src/generator/primitives/regions/heap_region.cpp` | 实现修改 | `generate_deallocation` 使用传入的 `size` 生成正确类型 |

---

## 验证步骤

1. `cd build && make`
2. `./mset --output-dir test_cases_mlir/`
3. 检查生成文件数量应与 C 版本一致
4. `mlir-opt --verify-diagnostics test_cases_mlir/xxx.mlir`
