# ADR-0015: Official research experiments and evaluation

**English** | [简体中文](0015-experiments-and-acceptance.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Default research configuration](0009-incremental-potential-rigid-contact.md), [multi-World simulation](0011-multi-world-batch-simulation.md), [diagnostics](0014-visualization-and-diagnostics.md).

## Context

Official research conclusions require identifiable models, plugin compositions, execution conditions and original results. Correctness, reproduction, failure handling and performance provide separate evidence for evaluating an experiment.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Visual inspection and aggregate speed | Useful for exploration; provides limited evidence for numerical and physical claims. |
| Property-based layered evaluation with reproducible experiments | Selected approach for official research; connects behavior and performance claims to explicit conditions and evidence. |

## Decision

### Scope and user ownership

The practices below apply to official research experiments. Users define their own acceptance metrics, thresholds, references and evaluation workflows. Each official experiment states its own criteria and conclusions.

The engine and its plugins provide diagnostics and extension mechanisms. Plugins report applicable state, residuals, iteration counts, timings and outcomes according to their protocols. Selected components define their own checks for their dependencies, backend support and data requirements; users manage the composition under ADR-0010. Solvers report convergence and failure reasons under their configured protocols; users decide whether results satisfy application requirements.

### Correctness

Validation covers basic calculations, physical laws, discrete solution accuracy, complete trajectories, plugin composition and multi-World isolation. Evidence includes analytic results, invariants, independent derivative checks, reference algorithms, high-accuracy calculations and step/tolerance refinement.

Each reference states its assumptions and uncertainty. External-engine comparisons record model and discretization differences. Numerical convergence and physical fidelity are assessed separately. Official experiments for the default configuration check zero-slip friction, hard-closure errors, friction admissibility and failure behavior. Experiments for other compositions select properties appropriate to their models.

### Reproduction

Records identify code/build/toolchain, hardware/backend, plugins and parameters, source and processed assets, initial state, inputs, overrides, random state and stepping/reset/retry history. A World from a batch can be extracted with its dependencies and inputs for individual reproduction.

Reproduction claims distinguish bitwise equality, numerical agreement within tolerances and statistical agreement. Validation examines repeated runs, standalone versus batched execution, worker assignments, restored continuation and cross-platform/backend comparisons. Each experiment states the claim being evaluated and its execution conditions.

### Failure handling

Official experiments record advancement outcomes and stopping reasons. Default-solver experiments check convergence against original residuals and feasibility conditions. Diagnostic evidence includes iteration limits, stagnation, line-search failure, invalid input and numerical exceptions.

Official experiments verify committed/trial state boundaries, complete plugin rollback, configured retry/reset behavior, per-World isolation and reproducible failure records. Experiments retain all attempts, failures and retries and report success rates and recovery cost.

### Multi-World performance

Performance is measured under documented result-quality requirements. Reports include accepted simulated work and duration, throughput, step/tail latency, CPU scaling, shared and per-World memory, initialization, partial reset, data exchange and workload imbalance. Contact representation, substeps, tolerances and failure policy accompany comparisons.

Setup/preprocessing, warmed simulation, end-to-end batch data exchange and optional diagnostic/viewing overhead are measured separately. Different accuracy settings are presented as speed–quality trade-offs with reliability statistics.

### Evidence and scope

Each experiment produces configuration, original results and a concise conclusion with passing properties, failures and limitations. Related tests use parameterization and shared fixtures; long scans and performance studies run separately. Validation selection follows the change's affected properties.

Specific scenes, thresholds, test interfaces, performance targets and storage formats remain undecided. This record establishes evaluation practices for official research. Implementation and experimental evidence remain pending.

## Consequences

Claims become traceable to explicit configurations and raw evidence. Reproduction metadata and failure accounting add work, while layered checks allow targeted validation and a controlled test-file count.

## Validation

Validation status: Pending implementation and experiment infrastructure.

Review representative reports for all four evidence categories. Verify that selected failures and batch instances can be reproduced, and that comparisons expose numerical settings, attempted work, accepted work and excluded measurements.
