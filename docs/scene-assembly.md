# Compiling a sphere and a plane

**English** | [简体中文](scene-assembly.zh-CN.md)

The installed C++ API in `HibikiPhysics/Scene.h` accepts a `SceneDescription` built in code or returned by `LoadScene(path)`. The native v1 text format uses decimal numbers, metres and a +Z up axis. Its first nonblank line is `hibiki-scene 1`. The remaining lines can contain one sphere and one horizontal plane in either order. Both shapes are optional. Separate fields with whitespace. Names are unique single tokens. Blank lines are ignored.

```text
hibiki-scene 1
plane ground 0 2 1
sphere ball 0.625 0.5 1 2
```

The plane fields are name, height, group and mask. The sphere fields are name, initial centre Z, positive radius, group and mask. Group and mask are unsigned 32-bit decimal bit fields. Each shape's group must intersect the other's mask for the pair to request the `collision.sphere-plane` capability. Unsupported records, versions, trailing fields and invalid dimensions cause load errors with the source path and line.

`CompiledScene(scene)` selects the default detector. To include contacts within a positive gap, copy `DefaultSceneConfig()`, call `Replace(SpherePlaneWithMargin("experiment.margin-0.25m.v1", 0.25))` on the copy, and pass it to the constructor. Give each experimental policy and parameter set a distinct implementation ID. A scoped factory registers the selected detector on the selected collision service. Releasing the compiled scene detaches that registration. A custom detector can use the public typed `SpherePlaneSlot` with `Borrow` or `Factory`. The selected provider receives the current sphere height on each `QueryContact(centerZ)` call. Its signed surface gap is centre Z minus radius minus plane height. The default returns a value at contact or penetration. The experiment also returns separations up to its margin. An ineligible pair returns an empty optional.

`GetSource()` retains the source identity, dimensions, authored pose, names and filters. `GetSelections()` reports the bound capabilities, implementation IDs and replacement flags. Later configuration edits apply to newly compiled scenes. The runnable example loads the same asset for both configurations and prints their selections and results. [Scene consumer](../examples/install-consumer/SceneConsumer.cpp) · [Native scene](../examples/install-consumer/sphere-plane.hscene)
