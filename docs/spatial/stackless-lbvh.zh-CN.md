# Morton BVH 无栈遍历

[English](stackless-lbvh.md) | **简体中文**

`CpuSpatialOverlapPlugin()` 为 CPU 粗筛选择无栈 Morton 树，实现名为 `hibiki.stackless_lbvh`。显式选择时，可包含 `<HibikiPhysics/StacklessSpatialIndex.h>` 并注册 `StacklessSpatialOverlapPlugin()`，也可直接调用 `MakeStacklessSpatialOverlapProvider()`。盒查询命中项与候选对均按遍历顺序输出，每对包含两个不同的调用方 ID，较小 ID 在前。[重叠查询契约](../geometry/spatial-index.zh-CN.md)

## 跳过子树

索引按先序构建 Morton 层次，然后为每个节点额外保存一个 `std::size_t` 跳转位置。根节点跳到数组末尾之后，左孩子跳到右兄弟，右孩子继承父节点的跳转位置。顺序扫描一次即可生成所有跳转位置。[Morton 构建过程](morton-lbvh.zh-CN.md)

单盒查询持有当前节点的下标。若包围盒不相交，就跳过该节点的整棵子树。若相交，遇到叶子时输出 ID，随后前进一个位置。自配对查询从每个源叶子的下一位置开始执行相同的遍历。后面的每个叶子在这段后缀中恰好出现一次。相交时输出两个 ID，并把较小的 ID 放在前面。除候选结果外，遍历使用常量大小的临时空间。

构造 `StacklessSpatialOverlapIndex index(entries)`，向 `index.Overlaps(bounds, visits)` 或 `index.OverlapPairs(visits)` 传入 `std::size_t` 计数器，即可取得本次调用测试节点包围盒的次数。包围盒测试失败也计一次。自配对计数累加每个源叶子的后缀遍历，选择源叶子的扫描无需测试包围盒。并发调用可以使用各自的计数器。[安装包回归测试](../../tests/install-consumer/StacklessSpatialIndex.cpp)

## 与显式栈对照

回归程序使用种子为 52 的 `std::mt19937` 生成 1,024 个盒子，在同一组输入上用同一 Morton 构建器分别构造两个索引。每个输入盒子都做一次查询，随后枚举全部自配对。参考遍历把右、左子节点依次压入向量。到达一个源叶子时，栈中剩余的子树恰好覆盖无栈算法扫描的后缀。程序逐盒比较排序后的候选 ID 和访问数，然后比较排序后的候选对及其总访问数。[对照程序](../../tests/spatial/StacklessTraversal.cpp)

| 工作负载 | 两种遍历的候选数 | 两种遍历的节点访问数 | 无栈耗时中位数（µs） | 显式栈耗时中位数（µs） |
| --- | ---: | ---: | ---: | ---: |
| 1,024 次单盒查询 | 3,360 | 46,462 | 361.0 | 580.4 |
| 全部自配对 | 1,168 | 17,432 | 86.8 | 104.7 |

测量环境为 Windows x64、Ryzen 7 9700X、clang-cl 22.1.6、Release。每次计时覆盖一轮完整工作负载，包含结果分配。程序先预热两种遍历，随后交替安排测量顺序，各运行九次取中位数。构树和结果核对在计时区间之外。可在准备好的 Windows 工具链中复现：

```powershell
cmake --preset windows-release -DHIBIKI_BUILD_ACCEPTANCE=OFF
cmake --build --preset windows-release --target hibiki_stackless_traversal
ctest --preset windows-release -R '^geometry\.stackless_traversal$' -V
```
