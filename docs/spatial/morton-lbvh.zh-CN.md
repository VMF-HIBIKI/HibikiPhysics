# Morton 线性 BVH

[English](morton-lbvh.md) | **简体中文**

CPU Morton 提供器为有限的闭合 AABB 构建只读层次结构，通过已有空间能力槽位支持范围候选查询、集合内部重叠对、射线候选查询和最近图元查询。结果使用调用者分配的原始 ID。[空间查询约定](../geometry/spatial-index.zh-CN.md)

## 选择提供器

调用者可在指定的能力作用域内替换默认的中位数 BVH。下面的示例请求完整查询能力，检查两个包围盒在顶点相接的情况。

```cpp
#include <HibikiPhysics/MortonSpatialIndex.h>
#include <array>

int main()
{
    using namespace HibikiPhysics;
    CapabilityConfig config;
    config.Add(CpuSpatialQueryPlugin());
    config.Replace(MortonSpatialQueryPlugin());
    Dependencies requirements;
    SpatialQuerySlot().Declare(requirements);
    const auto scope = config.Build(requirements);
    const auto &provider = SpatialQuerySlot().Get(scope);

    const std::array<SpatialEntry, 2> entries{{
        {7, {{0, 0, 0}, {1, 1, 1}}},
        {3, {{1, 1, 1}, {2, 2, 2}}}
    }};
    const auto index = provider.Build(entries);
    return index->OverlapPairs() == std::vector<std::pair<std::size_t, std::size_t>>{{3, 7}} ? 0 : 1;
}
```

只需重叠查询的调用者可为 `SpatialOverlapSlot()` 注册 `MortonSpatialOverlapPlugin()`。两个槽位均使用实现名 `hibiki.morton_lbvh`。独立使用时可调用 `MakeMortonSpatialQueryProvider()` 或 `MakeMortonSpatialOverlapProvider()`。使用 SDF 烘焙器时，将选定的查询提供器作为第三个参数传给 `BakeDenseSdf`。[安装包消费者测试](../../tests/install-consumer/MortonSpatialIndex.cpp)

## 构建层次结构

构建器先通过浮点中点运算求得各包围盒中心。每个坐标轴按该轴所有中心的范围做归一化，然后映射到 2²¹ 个桶，上端点归入最后一个桶。所有中心坐标相同的轴归入零号桶。如果区间宽度溢出，先将两端点和当前坐标同时减半，再计算差值和比例。

三个轴的桶编号按位交织为 63 位 Morton 码。叶节点先按 Morton 码排序，相同码再按无符号调用者 ID 排序。构建器取区间首尾 Morton 码的最高差异位分割区间。若首尾码相同，则使用首尾 ID 的最高差异位。输入排列不影响树结构，中心完全重合的图元也能独立成叶。父节点包围子节点的原始范围，量化仅影响分组。树按前序存储，每个输入对应一个叶节点，每次分割对应一个内部节点。

两种 CPU 构建器共用闭合包围盒相交判定、保守射线判定和最近距离遍历。候选 ID 和规范化的重叠对都按升序返回。最近查询只用叶节点的有限精确距离更新结果。相同距离取最小 ID，回调返回正无穷表示跳过该图元。
