# 批量重叠候选对

[English](batched-overlap.md) | **简体中文**

`CollectOverlapPairs(index, entries, settings, stats)` 使用建立只读索引时的原始条目查询候选。稳定 ID 决定从小到大的配对方向，各线程最多在本地积累 64 对后再原子预留一块输出区。首次缓冲区不足时仍统计完整数量，第二遍按该数量分配。最终结果按规范化 ID 排序。查询失败时先等待工作线程，再向调用方传播异常，成功后发布统计。

`BatchedMortonOverlapPlugin(settings)` 将此策略与 CPU Morton 树一起注册。配置指定总线程数和初始候选对容量。线程数为零时选用硬件并发数，调用线程参与计算，实际线程数受条目数限制。

## 高重叠样例

Release 安装包程序 `hibiki_batch_overlap` 在同一棵 Morton 树上查询 132 个重合盒，共 8,646 个无序对。Ryzen 7 9700X、Windows x64、clang-cl 22.1.6 下，七次独立进程测量的中位数为串行树遍历 0.3449 ms、4 线程批量收集 0.9222 ms。批量配置从两个槽位开始，两遍遍历后得到同样的集合，计时包含线程启动及扩容重试。
