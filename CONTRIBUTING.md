# Contributing to HibikiPhysics

**English** | [简体中文](CONTRIBUTING.zh-CN.md)

HibikiPhysics uses a lightweight workflow designed for a small, verifiable C++ physics engine. The maintainer owns scope, architecture decisions, review and release readiness.

## Planning work

Each substantial change starts with an issue containing:

1. The problem or intended capability.
2. The scope and relevant dependencies.
3. Observable acceptance criteria.
4. A verification approach.
5. Known risks and follow-up work.

Tasks typically cover one or two days of focused implementation. Exploratory work has a time budget and produces an experiment report or design decision.

During the local setup stage, the roadmap serves as the planning entry point. Issues and project-board entries can be created when remote hosting is configured.

## Milestones and task states

Milestones describe runnable, verifiable outcomes. The planned board states are:

| State | Meaning |
| --- | --- |
| Backlog | Work identified for consideration |
| Ready | Scope, dependencies and acceptance criteria are defined |
| In Progress | Implementation or investigation is active |
| In Review | The change and its evidence are ready for review |
| Done | The change is merged and its acceptance criteria are satisfied |

Keep up to two implementation tasks active at a time. Record status on the board, version scope in milestones, and classification in labels.

Suggested labels:

- Type: `feature`, `bug`, `design`, `maintenance`.
- Area: `core`, `collision`, `solver`, `tooling`, `docs`.
- Priority: `P0` for blockers, `P1` for milestone requirements, `P2` for follow-up work.

## Branches and commits

Use `main` as the integration branch and create short-lived topic branches such as:

```text
feat/12-body-handles
fix/41-contact-normal
docs/contribution-workflow
```

Use concise, descriptive commit subjects. Conventional prefixes such as `feat:`, `fix:`, `docs:` and `build:` identify the purpose of a change.

Each pull request addresses one coherent topic. Squash merging provides a concise integration history while the pull request retains discussion and verification evidence.

## Issue and pull request titles

Each implementation issue corresponds to one pull request. The issue and PR use exactly the same title, including category, description and language. A larger task is split into implementation issues, each with its own matching PR. The PR body links its issue with `Closes #<number>` when remote issues exist.

The title format is `type: description`: a lowercase category, an ASCII colon, one space, and a concrete description. Both titles are updated together when scope changes. The squash commit subject retains the same category and description; hosting may append the PR number.

| Category | Scope |
| --- | --- |
| feat | New capability |
| fix | Defect correction |
| refactor | Implementation restructuring with preserved externally observable behavior |
| perf | Performance improvement |
| test | Test additions or maintenance |
| docs | Documentation and design records |
| build | Build system, dependencies and toolchain |
| ci | Continuous integration and automation |
| chore | Repository maintenance |

Examples:

```text
feat: Add body handles
fix: Correct contact normal direction
refactor: Separate body storage from world orchestration
perf: Reduce broad-phase temporary allocations
build: Pin the Windows toolchain
```

Issue forms provide category guidance and a default prefix. The PR template records a title-match review item. These templates document the policy; automated title validation is a future CI task.

## Design decisions

Create an architecture decision record for choices with lasting impact, including ownership, public interfaces, numerical conventions, solver selection and backend boundaries.

Use [the ADR template](docs/decisions/0000-template.md). Include context, alternatives, the selected approach, consequences and validation. Link the record to the relevant issue and pull request when those resources exist.

## Pull requests and review

Open a draft pull request early for work spanning multiple days. Complete the pull request template with the problem, resulting behavior, implementation choices and verification evidence.

Describe API changes, simulated-physics changes and performance changes separately. Record numerical settings and measured results for changes that affect simulation behavior or performance.

The maintainer performs and records self-review during the single-maintainer phase. External reviews and AI-assisted checks are identified by their actual source. Configured automated checks must pass before merging. Branch protection and required checks will be configured when remote hosting and CI are established.

## Verification

Choose checks appropriate to the change:

| Change | Expected evidence |
| --- | --- |
| Documentation | Link, terminology and consistency review |
| Build or packaging | Clean configure/build and consumer installation check |
| Core C++ behavior | Focused unit or integration tests |
| Physics algorithm | Reference behavior, numerical tolerances and a reproducible scene |
| Bug fix | Minimal reproducer and regression test |
| Performance optimization | Correctness checks and comparable measurements |

Physics acceptance criteria identify the scene, timestep, precision, iteration settings and tolerances. Analytical solutions and convergence behavior provide reference evidence where applicable.

Benchmark reports include the commit SHA, hardware, compiler and dependency versions, build configuration, scene size, warm-up policy, timing boundaries, repetitions and numerical quality metrics. Warp experiments additionally identify compilation, transfers and synchronization within the measured workload.

Build and test commands will be documented alongside the corresponding working infrastructure.

## Commit checks

[ADR-0003](docs/decisions/0003-commit-hooks-and-validation.md) defines the accepted prek 0.5.9 workflow. Hook activation is scheduled for environment initialization. Fast pre-commit checks cover staged-file formatting, Python lint and file validity. Local fixes are reviewed and staged before retrying a commit. Full checks run explicitly and in PR CI; CI uses check-only modes. Specialized physics, sanitizer and benchmark checks follow the affected change and milestone scope. The editor, CLI and hooks share the pinned project tools.

## Definition of done

- The change satisfies the issue's acceptance criteria.
- Relevant verification results are recorded.
- Public behavior and usage documentation match the implementation.
- Design decisions and known limitations are documented where relevant.
- Review findings are resolved or tracked as explicit follow-up work.
- The change is merged into the integration branch.

## Documentation and development logs

Project documents, development logs and contribution templates have English and Simplified Chinese editions. English files retain their original names; Chinese files use the `.zh-CN` suffix. Reciprocal language links connect each pair. Content, status and links are updated together in the same change. Chinese pages prefer Chinese document links.

GitHub issue forms provide separate language entries. The default PR template links to both editions. Descriptions use declarative statements that express behavior, purpose, evidence and consequences directly. Labels and headings remain concise.

Weekly logs focus on a technical problem, experiments, results and the next step. Include links to design records, changes and reproducible evidence.

## Releases and attribution

Each release contains a version tag, release notes, reproduction instructions, supported environments and known limitations. Milestone acceptance determines readiness.

Original project code, documentation and official plugins/examples use [Apache-2.0](LICENSE). Contributions intended for inclusion follow its contribution terms. Independently developed plugins may choose their own license while satisfying applicable dependency terms. Record the origin and license of incorporated code and assets, and retain required notices when distributing them. Store larger videos and benchmark outputs as release attachments or CI artifacts once hosting is available.
