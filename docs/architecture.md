# Architecture direction

**English** | [简体中文](architecture.zh-CN.md)

Status: Overall direction accepted. M1 implementation is in progress.

## Current implementation

The C++20 static target `hibiki::physics` exports the build-information API and the public `HibikiPhysics::Plugin` base class. The Windows toolchain, editor, debugger and environment reproduction have passed validation. [Windows acceptance record](environment/acceptance.md).

Plugin authors include `<HibikiPhysics/Plugin.h>` and override `OnMount(PluginContext&)`. A plugin with dependencies also overrides `Declare(Dependencies&) const` and calls `Require<Protocol>(slot)`. The caller binds the chosen protocol objects by reference and calls `Plugin::Mount`. This entry point calls `Declare`, validates each required slot, then invokes `OnMount`. Plugin authors migrating an existing `Mount` override rename it to `OnMount`. The installed-package example declares an `int` dependency, binds it to a variable, and updates the variable through `Get<int>`. [Consumer source](../examples/install-consumer/PluginConsumer.cpp).

During mounting, `Keep` takes ownership of an RAII registration. The caller keeps the plugin and its providers alive until outstanding calls finish and the scope releases its registrations in reverse order. An installed-package integration test exercises a concrete plugin, provider selection and scope cleanup. [Lifecycle test](../tests/install-consumer/PluginLifecycle.cpp).

## Accepted directions

| Direction | Responsibilities and boundaries |
| --- | --- |
| Open plugins | Users define protocols, data and complete execution flows. Official and user plugins share extension mechanisms. A common lifecycle supports resource management. Users manage versions, dependencies and composition. |
| Scenes and assets | A native intermediate parsing layer receives programmatic construction and format adapters; users can extend formats. |
| World lifecycle | Configuration, scene compilation and execution. Structure and model parameters are fixed after compilation; interaction uses explicitly enabled inputs. |
| Multiple Worlds | Compatible read-only model data is shared with independent instance state. Per-World parameters and domain randomization are determined before compilation. |
| Default research composition | CPU incremental-potential rigid bodies, reduced-coordinate trees and free poses, ideal hard loops, compliant normal contact and semismooth Newton nonsmooth static friction. |
| Time and restoration | Default synchronous batch interaction, extensible time advancement, and partial-reset/snapshot workflows for the official composition. |
| Error handling | Official diagnostic and error-handling example plugins; users design error categories, propagation, state validity and recovery. |
| Entry points | Independent C++, local Python construction/simulation/viewing, and CLI reproducible experiments. |
| Visualization and backends | External renderers, a CPU-first research path and incremental optional Warp backend support. |

These directions are documented in the [ADR index](decisions/README.md). The default research composition is an official configuration; users may select other models, solvers and flows. Evaluation practices apply to official research; users determine application acceptance.

## Subsequent interface design

The scene compiler and plugin host will resolve user-selected providers and run component-defined checks before mounting plugins. Handle representation, data layout, mathematical protocols, Python bindings and scheduling follow M1 and later module research. Sensors retain the plugin direction with concrete coverage pending investigation. The SDF approach and implementation require separate research.

## Implementation path

The [eight-week plan](plans/0002-cpu-research-release.md) proceeds through runtime skeleton, free bodies, contact/friction, multibodies/joints and usage workflows. Multiple Worlds and extension examples run through implementation; actual coverage is recorded by milestone.

Current build/install operations are documented in the [build guide](environment/build.md), with preparation in the [environment guide](environment/prepare.md). The static-library and Windows baseline follows the [environment plan](plans/0001-windows-environment.md); shared-library support remains a subsequent extension.
