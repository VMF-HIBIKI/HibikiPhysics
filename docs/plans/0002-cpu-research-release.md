# Eight-week CPU research release plan

**English** | [简体中文](0002-cpu-research-release.zh-CN.md)

Status: Accepted; implementation pending kickoff.

Date: 2026-11-01

## Objective and scope

Eight effective development weeks begin at implementation kickoff, building on the verified Windows environment and build. The target is a CPU research release delivered as a C++20 static library, connecting scene compilation, multi-World simulation, default rigid-body solving and local viewing. The version target is v0.1.0; milestone completion follows actual results.

Core delivery includes open plugins and a common lifecycle, independent World state, the default CPU solver path, official examples and experiment records. Priority extensions include articulated trees, basic joints, simple loops, local Python operation and an external renderer adapter. Warp simulation, CCD, differentiability, concrete sensors, the SDF approach and full joint/robot-format coverage remain subsequent research.

Broad shape coverage remains the long-term direction. Initial implementation covers necessary sphere, plane and box collision pairs; convex geometry and further shapes follow progress. Each shape records parsing, collision, simulation and viewing coverage separately.

## M1: Runtime skeleton, week 1

- C++ scene configuration, per-World parameters and scene compilation.
- Plugin initialization, resource ownership, teardown and user-defined data and execution flows.
- Single-World and synchronous batch advancement with independent state and simulation time.
- An example-defined runtime-input channel and effective timing.
- An independent plugin with its own state and update rules, assembled by the example application.
- An initial error-handling example: a plugin emits diagnostics and the application chooses whether to stop or continue.

Deliver the static library and three small examples for single-World, multi-World and external-plugin use. Development checks cover lifecycle, isolation and fixed compiled configuration. Users manage plugin versions, dependencies and composition; components define their own required checks.

## M2: Free rigid bodies, week 2

- Required vector, matrix, quaternion and pose operations; explicit units, frames and angular-velocity semantics.
- Mass, center of mass, inertia, initial pose/velocity, gravity and external-force input.
- Default translational and rotational incremental potentials, internal discrete-state initialization and updates.
- Gradients, Hessians, Newton iterations, line search and stopping information.
- Multiple Worlds with different initial states and pre-compilation parameters.

Experiments cover free fall, principal-axis rotation, general free rotation and external forces. Records include derivative checks, step-size studies, energy and angular momentum. Backward Euler comparisons first specify the compared formulations, equations and state conventions; an independent reference implementation follows research needs.

## M3: Contact and static friction, weeks 3–4

- Local shape transforms, bounding volumes, candidate generation and collision filtering.
- Necessary sphere, plane and box collision combinations, with explicit pair coverage.
- Contact points, normals, gaps, tangent frames and history.
- Compliant normal-contact potential and derivatives.
- Zero-slip static friction, sliding friction, projection residuals and generalized derivatives.
- Semismooth Newton, line search, redundant multi-contact handling and outer contact updates.
- Residuals, iteration counts, contact states and failure context.

Representative experiments include falling spheres and boxes, sub-limit pushes, sliding, inclined-plane sticking and simple stacks. Pending models such as normal damping receive separate discussion before implementation.

At week 4, review static-friction loading, multi-contact convergence and failures. Adjust convex geometry, additional shapes and later extensions using this evidence, recording reasons and effects.

## M4: Multibodies and joints, weeks 5–6

- Compile links, fixed/floating bases, articulated trees and additional closure connections.
- Fixed connections, revolute joints and prismatic joints.
- Forward kinematics, motion mappings and derivatives required by the default solver.
- Map link inertia, external forces and contact into mechanism variables.
- Ideal hard loop constraints, initialization checks, constraint residuals and redundancy diagnostics.
- Joint force/torque inputs and explicitly supported external forces.
- A native intermediate parsing layer for representative mechanisms and subsequent format adapters.

Experiments include a pendulum, serial double pendulum, prismatic mechanism, four-bar linkage and a mechanism contacting the ground, including multi-World runs. Limits, position drives, joint dry friction and elastic connections follow their specific decisions and available progress, with individual completion records.

## M5: Usage and research delivery, weeks 7–8

- Local Python configuration, scene compilation, advancement, input submission and result access.
- CLI execution of supplied experiments, configuration selection and result export.
- An explicit subset of one selected robot format, with provenance and import diagnostics.
- One external renderer adapter for geometry, poses and selected diagnostics, with World selection.
- Partial reset to configured initial state while retaining compiled parameters.
- Snapshots and restoration for the official plugin composition, covering individual and selected Worlds.
- An error-handling example plugin, outcome summaries and caller-selected response examples.
- Recorded experiment configurations, execution conditions, raw results, conclusions, known issues and reproduction steps.

Initial snapshots target continuation under the same build and compatible configuration. Cross-version migration, complex asynchronous recovery and general persistence formats remain subsequent work. Mouse-applied force is an optional input extension; scene editing enters configuration and recompilation. Select the concrete robot format and renderer before their implementation stage. These weeks also cover integration and fixes.

## Cross-cutting requirements and research risks

Multi-World execution and plugin extensibility run through all milestones from M1, retaining shared read-only models, independent state and pre-compilation per-World parameters. Initial interaction uses synchronous batches; asynchronous scheduling remains subsequent implementation. Model structure and parameters follow [ADR-0019](../decisions/0019-world-configuration-and-runtime-inputs.md).

Users design plugin management, error categories and recovery policies. Official plugins and examples provide a reference composition; concrete protocols and interfaces follow module design. Evaluation follows the official research scope of [ADR-0015](../decisions/0015-experiments-and-acceptance.md); users define application acceptance criteria.

Contact/friction and multibody coupling are the main research risks. Milestone reviews adjust extension scope using actual execution evidence. Focused checks and parameterized experiments keep the test-file count controlled.

The final support matrix records actual shape, joint, plugin lifecycle/reset/snapshot and backend capabilities. The release candidate includes the buildable library, representative experiments, Python and external-viewer workflows, reproduction records and known issues. Remote hosting and public release follow subsequent arrangements.
