# ADR-0020：几何库插件

[English](0020-geometry-library-plugin.md) | **简体中文**

状态：已接受

日期：2026-11-21

相关工作见 [Issue #17](https://github.com/VMF-HIBIKI/HibikiPhysics/issues/17) 和 [ADR-0010](0010-plugin-architecture-and-lifecycle.zh-CN.md)。

## 背景

HibikiPhysics 需要让研究插件共用几何定义。Jolt 提供复合形状与变截面几何，MuJoCo 定义模型使用的 geom 类型，MotrixSim 区分源几何与生成的碰撞体，Isaac Sim 允许同一网格选择多种碰撞表示。项目据此分别记录几何表达、格式导入和碰撞支持范围。[Jolt 形状类型](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/Collision/Shape/Shape.h) · [MuJoCo 几何](https://mujoco.readthedocs.io/en/stable/XMLreference.html#body-geom) · [MotrixSim 几何 API](https://motrixsim.readthedocs.io/en/v0.10.0/api_reference/core/motrixsim.html) · [Isaac Sim 碰撞表示](https://docs.isaacsim.omniverse.nvidia.com/latest/physics/simulation_fundamentals.html)

## 关键选择

一个官方几何库插件可以提供多种几何。插件负责形状定义、局部坐标和尺寸约定。格式适配器负责源数据转换。碰撞插件负责形状配对与接触算法。插件核心沿用 ADR-0010 的能力装配机制。

下表是已接受的候选清单，各类型的实现与验证进度分别记录。

| 类别 | 候选几何 |
| --- | --- |
| 基础解析几何 | 球、盒、胶囊、圆柱、椭球、平面或半空间 |
| 扩展解析几何 | 圆锥、圆台、两端半径不同的胶囊、有限矩形面片 |
| 离散与凸几何 | 三角形、凸包、三角网格 |
| 地形与场 | 高度场、有符号距离场（SDF） |
| 实例与组合 | 带局部位姿的几何实例、复合几何、各形状的缩放规则 |

基础解析几何任务将确定平面的边界与内部语义、盒采用半尺寸或全尺寸、胶囊采用圆柱段长度或端到端总长，以及局部轴向。SDF 的表示与生成方式留待专项研究。

## 注册与验证

应用链接 `hibiki::physics`，包含 `<HibikiPhysics/Geometry.h>`，通过 `CapabilityConfig::Add` 注册 `GeometryLibraryPlugin()`。消费方通过 `GeometryLibrarySlot()` 声明依赖。官方工厂为每个请求该能力的作用域创建一个 `GeometryLibrary` 实例，其当前公开职责是标识几何模块的服务。首批形状定义和几何运算作为单独任务交付。

安装包测试验证显式注册、选定对象的注入、各作用域的独立工厂实例，以及当前配置中的实验替换。[几何插件测试](../../tests/install-consumer/GeometryPlugin.cpp)
