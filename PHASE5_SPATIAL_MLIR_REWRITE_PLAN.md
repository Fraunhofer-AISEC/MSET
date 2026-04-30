# Phase 5: 重写 bug_types/spatial/ 为 MLIR 语义

## 1. 目标

将 `bug_types/spatial/` 及其依赖的 `Flow`、`OriginTargetRelation` 从 C 语义迁移为 MLIR 语义。

**范围**：
- `bug_types/spatial/flow/`
- `bug_types/spatial/origin_target_relation/`

上游 `AccessLocation`、`Region` 已 MLIR 化，只需适配接口；下游 `generator.cpp` 无需改动。

---

## 2. 核心设计决策

### 2.1 Distance 策略

| 关系 | 策略 |
|------|------|
| **InterObject** | 生成时随机：`std::mt19937`，范围 `[0, MAX_DISTANCE]`，MLIR 中写成 `arith.constant N : index` |
| **IntraObject** | 固定 `distance = origin_size`，对齐字节数 |
| **NonObject** | 上溢 `distance = origin_size`，下溢 `distance = -1` |

### 2.2 为什么 Region/AccessLocation 不用动态化

`rand_distance` 在 C++ 生成期确定，`total_size = origin_size + rand + target_size` 在生成期就是常量。MLIR 中仍是 `memref<total_size xi8>` 静态 shape，只是不同测试文件的数字不同。

### 2.3 关键原则

- **Single-parent-memref + subview**：`InterObject` / `IntraObject` 统一在一个父 memref 内切分 origin/target
- **消除地址计算**：移除 `GET_ADDR_BITS`、指针算术、C 结构体
- **Precondition → `arith.cmpi` + `scf.if` + `func.return`**
- **Counter → `arith.addi` / `arith.subi`**
- **TypeConfusion → `memref.reinterpret_cast`**

---

## 3. 分步实施

### Step 1: `Flow` 接口 MLIR 化

**文件**：`flow/flow.h`, `flow/overflow.cpp`, `flow/underflow.cpp`

| 方法 | 原返回 | 新返回 |
|------|--------|--------|
| `generate_counter_update(cnt)` | `std::string` (C 表达式) | `std::vector<std::string>` (MLIR 行) |
| `generate_preconditions_check_distance(distance)` | `std::string` (C 表达式) | `std::vector<std::string>` (MLIR 行) |
| `generate_preconditions_check_in_range(x, from, to)` | `std::string` (C 地址比较) | **直接移除** (返回空 vector) |

**CodeCanvas 新增常量**：
```cpp
"%precond_fail = arith.constant 43 : i32"
"%test_success = arith.constant 42 : i32"
```

**示例**：
```cpp
// Overflow::generate_preconditions_check_distance
return {
  "  %is_valid = arith.cmpi sge, %" + distance + ", %c0 : index",
  "  scf.if %is_valid {",
  "    func.return %precond_fail : i32",
  "  }",
};
```

### Step 2: `OriginTargetRelation` 重写

**文件**：`origin_target_relation/origin_target_relation.h`, `inter_object.cpp`, `intra_object.cpp`, `non_object.cpp`

**`OriginTargetCodeCanvas` 新增字段**：
```cpp
std::string distance_ssa_name;         // e.g. "distance"
std::string distance_negated_ssa_name; // e.g. "distance_negated"
ssize_t distance_static_value;         // e.g. 137 (生成期已知，用于 C++ 判断)
```

**`InterObject::generate`**：
```cpp
#include <random>
static std::mt19937 gen(std::random_device{}());
std::uniform_int_distribution<> dist(0, MAX_DISTANCE);
int rand_distance = dist(gen);

size_t total_size = origin_size + rand_distance + target_size;
size_t target_offset = origin_size + rand_distance;

// 生成的 MLIR:
// %parent = memref.alloc() : memref<total_size x i8>
// %origin = memref.subview %parent[0][origin_size][1] ...
// %target = memref.subview %parent[target_offset][target_size][1] ...
// %distance = arith.constant rand_distance : index
// %distance_negated = arith.subi %c0, %distance : index
```

**`IntraObject::generate`**：
- 利用 `Region::generate(canvas, name, field1, size1, field2, size2)` 分配父 memref
- `distance_static_value = origin_size`（+ padding 若需对齐）

**`NonObject::generate`**：
- 只分配 origin memref
- 上溢：`distance_static_value = origin_size`
- 下溢：`distance_static_value = -1`

### Step 3: `LinearOOBA` 重写

**`generate`**：
```cpp
std::string distance = origin_target_canvas->get_distance();           // SSA value 名
ssize_t distance_static = origin_target_canvas->get_distance_static_value();

if (!flow->accepts_static_distance(distance_static)) continue;

// 用 distance（SSA 名）调用 AccessLocation
reach_target_codes = access_location->generate_bulk_split_all(
    access_action, origin_name, target_name, distance,
    generate_preconditions_check_distance, nullptr, generate_counter_update
);
```

> 指针追逐在 MLIR `memref` 中不可表达（无地址比较操作），改为 `scf.for` 确定性循环，但**循环结构保留**。

**`generate_validation`**：
- 可保留 `generate_bulk_split_using_index(..., distance="0", ...)`（空循环），再访问 target
- 或直接 `access_location->generate(..., target_name, target_size)` 访问合法 target

### Step 4: `NonLinearOOBA` 重写

**`generate`**：
```cpp
access_location->generate_at_index(
    access_action, origin_name, distance, target_size, precond_check
);
```

**`generate_validation`**：
```cpp
std::string var_name = origin_target_canvas->is_target_allocated() 
    ? origin_target_canvas->get_target_name() 
    : origin_target_canvas->get_origin_name();

access_location->generate_at_index(
    access_action, var_name, "0", target_size, nullptr
);
```

生成的 MLIR：
```mlir
// InterObject/IntraObject
scf.for %j = %c0 to %c8 step %c1 {
  %val = memref.load %target[%j] : memref<8xi8>
}

// NonObject
scf.for %j = %c0 to %c8 step %c1 {
  %val = memref.load %origin[%j] : memref<8xi8>
}
```

### Step 5: `TypeConfusion` 重写

**移除**：`struct BigType`、C 类型转换、`MAX_OBJECT_SIZE`

**BigType cast 变体**：对 **origin** 做 `reinterpret_cast`，将其"看大"为覆盖整个 parent：
```mlir
//假设生成的rand为137，所以总size是137 + 8 + 8 = 153.实际实现过程不可以硬编码
%parent = memref.alloc() : memref<153xi8>
%origin = memref.subview %parent[0][8][1] : memref<153xi8> to memref<8xi8>

%big_origin = memref.reinterpret_cast %origin to offset: [0], sizes: [153], strides: [1] 
                : memref<8xi8> to memref<153xi8>
%val = memref.load %big_origin[%distance] : memref<153xi8>
```

**Load widening 变体**：复用 `DirectLocation::generate_uint32`

**`generate_validation`**：
- BigType 变体：直接访问合法 target subview
- Load widening 变体：对 origin 做合法 narrow load (`generate_uint8`)

### Step 6: `AccessLocation` Callback 适配

**文件**：`access_location.h`, `direct_location.cpp`, `stdlib_location.cpp`

Callback 类型：
```cpp
// BEFORE
std::function<std::string(const std::string&)> generate_preconditions_check_distance

// AFTER
std::function<std::vector<std::string>(const std::string&)> generate_preconditions_check_distance
```

Precondition 结果作为**独立代码行**插入到 access 操作之前。

---

## 4. 接口变更对照表

| 接口 | 变更 | 影响文件 |
|------|------|---------|
| `Flow::generate_counter_update` | `std::string` → `std::vector<std::string>` | `flow.h`, `overflow.cpp`, `underflow.cpp` |
| `Flow::generate_preconditions_check_distance` | `std::string` → `std::vector<std::string>` | 同上 |
| `Flow::generate_preconditions_check_in_range` | **移除** | 同上 |
| `OriginTargetCodeCanvas` | 新增 `distance_ssa_name`, `distance_static_value` | `origin_target_relation.h`, 各 `.cpp` |
| `AccessLocation` callbacks | `std::function<std::string(...)>` → `std::function<std::vector<std::string>(...)>` | `access_location.h`, `direct_location.cpp`, `stdlib_location.cpp` |

**不变**：`Region::generate(..., size_t size, ...)`, `AccessLocation::generate(..., size_t size, ...)`

---

## 5. 风险点

1. `distance_static_value` 替代 `is_number(distance)` 做 C++ 层面判断
2. Validation 文件需与 generate 使用**相同的随机 distance**
3. `IntraObject` 当前 `char` 对齐为 1，未来其他类型需加 padding 计算

---

## 6. 文件修改清单（按依赖顺序）

1. `code_canvas.cpp` — 新增 `%precond_fail`, `%test_success`
2. `flow/flow.h` — 改接口
3. `flow/overflow.cpp`, `flow/underflow.cpp` — 实现新接口
4. `origin_target_relation/origin_target_relation.h` — 改 `OriginTargetCodeCanvas` 字段
5. `origin_target_relation/inter_object.cpp` — 重写（随机 distance + parent memref）
6. `origin_target_relation/intra_object.cpp` — 重写（固定 distance + 对齐）
7. `origin_target_relation/non_object.cpp` — 重写
8. `access_types/access_location.h` — 改 callback 签名
9. `access_types/direct_location.cpp`, `stdlib_location.cpp` — 适配 callback
10. `linear_ooba.cpp` — 重写
11. `non_linear_ooba.cpp` — 重写
12. `type_confusion.cpp` — 重写
13. 编译验证

---

## 7. 示例

`InterObject` + `LinearOOBA`，`rand_distance = 137`：

```mlir
//假设生成的rand为137，所以总size是137 + 8 + 8 = 153.实际实现过程不可以硬编码

%parent = memref.alloc() : memref<153xi8>
%origin = memref.subview %parent[0][8][1] : memref<153xi8> to memref<8xi8>
%target = memref.subview %parent[145][8][1] : memref<153xi8> to memref<8xi8>

%distance = arith.constant 137 : index

%is_valid = arith.cmpi sge, %distance, %c0 : index
scf.if %is_valid {
  func.return %precond_fail : i32
}

scf.for %i = %c0 to %distance step %c1 {
  %val = memref.load %origin[%i] : memref<8xi8>
}

memref.dealloc %parent : memref<153xi8>
```
