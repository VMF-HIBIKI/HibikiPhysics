# CPU 并行烘焙测量

[English](parallel-baking.md) | **简体中文**

在 AMD Ryzen 7 9700X、Windows x64、clang-cl 22.1.6 Release 上，0.25 mm 间距、XY 半格错位的通孔板用 4 个线程烘焙需 0.5375 秒，单线程需 1.8949 秒。该配置的 3,305,124 个节点在六次运行中得到相同的全网格指纹，轴向预测余量为 +0.075 mm。[精度实验](plate-accuracy.zh-CN.md) · [原始测量](parallel-plate-results.json)

| 间距（mm） | XY 相位（格） | 单线程（秒） | 4 线程（秒） | 加速比 | 进程峰值 MiB（1 / 4 线程） |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 0.0389 | 0.0132 | 2.95× | 5.21 / 5.12 |
| 1 | 0.5 | 0.0355 | 0.0110 | 3.22× | 4.72 / 5.14 |
| 0.5 | 0 | 0.2581 | 0.0740 | 3.49× | 7.49 / 7.90 |
| 0.5 | 0.5 | 0.2517 | 0.0706 | 3.57× | 7.54 / 7.97 |
| 0.25 | 0 | 1.9020 | 0.5550 | 3.43× | 29.27 / 29.68 |
| 0.25 | 0.5 | 1.8949 | 0.5375 | 3.53× | 29.50 / 29.93 |

每个间距和相位组合下，每种线程数在独立进程中测量三次，线程数的运行顺序交替。时间取烘焙调用的中位数，加速比取两组中位数之比。内存取该组合各次运行中最大的进程峰值工作集。机器负载、网格尺寸和可分配切片数都会影响结果。[线程选择和失败处理](serial-baking.zh-CN.md)

## 复现

```powershell
./scripts/environment/VerifyCpp.ps1 -Presets windows-release
python experiments/sdf/measure_accuracy.py build/installed-tests-windows-release/hibiki_sdf_accuracy.exe --workers 1 4 --repeats 3 --output parallel-plate-results.json
```

脚本在不同线程数及各次重复中核对全部双精度节点的指纹，涵盖外扩区域。安装包回归用例另以不能均分的切片逐节点核对位模式，并验证后台线程和调用线程出错后等待其他线程退出再传播异常。
