# Architecture direction

**English** | [简体中文](architecture.zh-CN.md)

Status: Overall direction accepted; concrete module interfaces pending design.

## Current implementation

The repository provides the C++20 static target `hibiki::physics`, a build-information API, an installation consumer example and environment checks. Windows toolchain, editor, debugging and environment reproduction have passed validation; see the [acceptance record](environment/acceptance.md). Physics algorithms, World APIs and runtime plugins remain subsequent implementation.

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

Module and build-target boundaries, handle representation, data layout, mathematical protocols, plugin signatures, Python bindings and scheduling follow M1 and later module research. Sensors retain the plugin direction with concrete coverage pending investigation. The SDF approach and implementation require separate research.

## Implementation path

The [eight-week plan](plans/0002-cpu-research-release.md) proceeds through runtime skeleton, free bodies, contact/friction, multibodies/joints and usage workflows. Multiple Worlds and extension examples run through implementation; actual coverage is recorded by milestone.

Current build/install operations are documented in the [build guide](environment/build.md), with preparation in the [environment guide](environment/prepare.md). The static-library and Windows baseline follows the [environment plan](plans/0001-windows-environment.md); shared-library support remains a subsequent extension.
