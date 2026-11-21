# 编译球体与平面场景

[English](scene-assembly.md) | **简体中文**

安装后的 C++ 头文件 `HibikiPhysics/Scene.h` 提供 `SceneDescription`。可用代码直接构造，也可用 `LoadScene(path)` 从文件读取。原生 v1 文本格式使用十进制数，以米为单位，+Z 向上。首个非空行须为 `hibiki-scene 1`。其后可按任意顺序各写一个球体和一个水平面，两者均可省略。字段以空白分隔，形状名称是唯一的单个词元。读取时忽略空白行。

```text
hibiki-scene 1
plane ground 0 2 1
sphere ball 0.625 0.5 1 2
```

平面的字段依次为名称、高度、分组和掩码。球体的字段依次为名称、初始球心 Z 坐标、正半径、分组和掩码。分组和掩码均为十进制无符号 32 位位字段。双方的分组均与对方的掩码有交集时，场景才请求 `collision.sphere-plane`。不支持的记录或版本、多余字段、无效尺寸会触发含源路径和行号的加载错误。

`CompiledScene(scene)` 使用默认检测器。若要检测相隔一定距离的球体和平面，可复制 `DefaultSceneConfig()`，在副本上调用 `Replace(SpherePlaneWithMargin("experiment.margin-0.25m.v1", 0.25))`，再将副本传入构造函数。每种实验策略和参数组合应有独立的实现标识。作用域工厂将所选检测器登记在所选碰撞服务上，销毁场景时会撤销登记。自定义检测器可以通过公开的类型化入口 `SpherePlaneSlot` 使用 `Borrow` 或 `Factory`。每次调用 `QueryContact(centerZ)`，已绑定的检测器都会收到当前球心高度。有符号间距等于球心 Z 坐标减去半径和平面高度。默认检测器在接触或穿透时返回该值。实验检测器还会返回指定容差以内的正间距。经过滤排除的形状对返回空值。

`GetSource()` 保留来源、尺寸、初始位姿、名称与过滤字段。`GetSelections()` 提供已绑定的能力、实现标识和替换标记。后续配置编辑只作用于新编译的场景。可运行示例用两种配置加载同一个资产，并打印选择与查询结果。[场景示例](../examples/install-consumer/SceneConsumer.cpp) · [原生场景](../examples/install-consumer/sphere-plane.hscene)
