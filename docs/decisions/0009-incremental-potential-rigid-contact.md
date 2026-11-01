# ADR-0009: Default rigid-body research configuration

**English** | [简体中文](0009-incremental-potential-rigid-contact.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Architecture direction](../architecture.md), [build and numerical policy](0004-build-and-numerical-policy.md).

## Scope

This record defines the official default research configuration for rigid-body dynamics, contact and friction. Its coordinate choices, force laws, solver policies and acceptance criteria apply to that composition. [ADR-0010](0010-plugin-architecture-and-lifecycle.md) defines the open plugin architecture and shared lifecycle. Alternative compositions can provide their own protocols, data, models, solvers and execution flows, together with explicit compatibility and validation requirements. Concrete mathematical extension interfaces remain undecided.

The default configuration is assembled through the same public extension mechanisms available to users. The decisions below remain accepted within this scope.

## Context

HibikiPhysics adopts a research focus on implicit rigid-body dynamics, contact modeling and nonlinear solution methods. The CPU implementation provides the reference formulation for controlled experiments. SuperDex provides a reference for rotational incremental potentials and compliant surface contact.

The design discussion used free rotation of a cuboid, followed by normal contact, zero-slip static friction and multi-contact solution on a plane. Implementation and numerical validation are subsequent work.

## Alternatives

| Approach | Role and trade-off |
| --- | --- |
| Incremental-potential rigid inertia with compliant contact | Selected CPU formulation; combines inertia and conservative contact in a scalar objective with consistent derivatives. |
| Direct Newton–Euler residual discretization | Candidate dynamics comparison; requires its own residual, Jacobian and globalization policy. |
| Sequential impulses | Candidate later contact comparison with distinct contact and time-discretization assumptions. |

## Decision

### State and time discretization

The model uses center-of-mass translation and rigid orientation. Quaternions store orientation; local three-component rotation increments define optimization updates. Each independent dynamic body contributes six local motion variables. Articulated links obtain their poses from root and joint coordinates.

Backward Euler is the initial time-discretization baseline. The selected incremental potential is a Backward-Euler-type formulation. Comparisons with later integrators retain this baseline and specify the inertia model, initialization, contact law, time step and solver tolerances. A direct Newton–Euler Backward Euler experiment is a separately specified candidate formulation.

The rotational state preserves the finite-difference information required by the selected inertia model. Physical angular-velocity inputs, internal discrete velocities and step-equivalent angular velocities have explicit semantics. Initialization and reconstruction policies remain subjects for a dedicated decision; continuous-tangent initialization used in the worked examples is a candidate.

### Hybrid multibody coordinates and connections

Articulation trees use reduced joint coordinates. Independent rigid bodies use free poses. A floating articulation adds a free root pose; a fixed-base articulation has a prescribed root transform. Forward kinematics maps the primary state `q` to each link's center-of-mass pose `X_i = K_i(q)`. Joint manifolds distinguish stored coordinates from local update dimensions. Derived link poses support collision and rendering.

Tree-joint geometry is satisfied by the kinematic parameterization. Ideal loop closures and ideal cross-mechanism connections use explicit hard equations `g(q) = 0`, enforced to documented solver tolerances. Elastic connections use explicitly configured compliant potentials and, when specified, consistent dissipation models. These connection semantics are selected explicitly.

Closed mechanisms are represented by a spanning tree and additional closure edges. Each closure contributes only the geometric conditions of its joint type. Consistent initial configurations, assembly branches, redundant rows and singular mechanism poses receive explicit validation and diagnostics. Connections and mechanism topology are fixed during scene compilation. Changes enter a new configuration and compilation cycle under [ADR-0019](0019-world-configuration-and-runtime-inputs.md).

Independent bodies and articulation links share rigid inertia, geometry and contact terms. Each link's discrete inertia potential is evaluated at its forward-kinematic pose, with the fixed predictors defined by the time integrator. Generalized derivatives follow the chain rule. In consistent local charts, `grad_q E = A^T grad_X E`, and the exact Hessian includes both `A^T H_X A` and the second derivatives of the kinematic map weighted by the link-space gradient. Approximate direction matrices retain explicit labels and original residual checks.

Contact positions and relative tangential displacements are evaluated through the same kinematic maps for all participating bodies. Their Jacobians map mixed free-pose and joint increments into contact motion. Coupled bodies and mechanisms enter one joint solve for their connected system.

Small maximal-coordinate mechanisms provide a reference path using the same rigid potentials and explicit joint constraints. This path verifies kinematic derivative transformations and constrained motion. The reduced-tree/free-body hybrid is the primary organization. Joint limits, drives and joint friction require their own subsequent force or constraint specifications.

### Incremental potential

For a body with mass `m`, center `x`, orientation `R`, time step `h`, mass second moment `J`, and fixed predictors `x_tilde` and `R_tilde`, the objective contains:

```text
translation inertia = m / (2 h²) * ||x - x_tilde||²
rotation inertia    = 1 / (2 h²) * tr((R - R_tilde) J (R - R_tilde)^T)
smooth base E       = translation inertia + rotation inertia + contact potential
```

Constant gravity may be incorporated into `x_tilde = x_n + h v_n + h² gravity`. This convention accounts for gravity once. Predictors remain fixed throughout the nonlinear solve for a time step.

### Frictionless compliant contact

Surface sample `i` has a fixed body-space offset `r_i` and an effective stiffness `k_i` that includes its quadrature weight. For a plane through the origin with unit normal `n`, its signed gap is `d_i = n^T (x + R r_i)`.

The reference contact potential is `U_i = 0.5 k_i max(0, -d_i)²`. This finite-stiffness model permits penetration and produces normal forces. Surface sample selection, quadrature and the smoothing policy will be specified during contact implementation. The piecewise quadratic model is continuously differentiable, with a second-derivative transition at zero gap.

For local variables `q = (delta_x, delta_theta)`, let `a_i = R r_i`, `j_i = (n, a_i × n)` and `B_i` be the gap Hessian. The derivative contract is:

```text
contact gradient = U_i' * j_i
contact Hessian  = U_i'' * j_i j_i^T + U_i' * B_i
```

The gap Hessian has a rotational block `sym(n a_i^T) - (n^T a_i) identity` and zero translation and mixed blocks for this plane model. The outer-product term supplies translation–rotation coupling. The negative contact gradient yields the normal force and its moment about the center of mass.

### Static-friction contract

For sample `i`, let `p_i(q)` be its current material-point position, `p_i^0` its step-start position, and `T_i` the orthonormal tangent basis. The two-component tangential displacement is `s_i = T_i^T (p_i(q) - p_i^0)`.

During an inner solve, the normal load `Nbar_i >= 0` is fixed and the friction limit is `c_i = mu_i Nbar_i`. The two-component resistance variable is `lambda_i`; the physical tangential force is `-T_i lambda_i`. The law is:

```text
sticking: s_i = 0, ||lambda_i|| <= c_i
sliding:  lambda_i = c_i s_i / ||s_i||, for ||s_i|| > 0
```

Zero slip refers to the discrete material-point displacement during a time step. Static-friction acceptance includes both instantaneous step tolerances and accumulated drift over sustained loading. A single friction coefficient defines the initial Coulomb law.

### Incremental potential and joint residual

Let `E(q)` contain inertia, conservative normal contact and any separately accepted smooth terms. For fixed loads and tangent geometry, the objective is:

```text
minimize Phi(q) = E(q) + sum_i c_i ||s_i(q)||
subject to g(q) = 0 for ideal closures and connections
```

The norm retains its set-valued subgradient at zero. Its subgradient relation is equivalent to the stated sticking/sliding law.

Let `J_i = partial s_i / partial q`, `C_i = {lambda : ||lambda|| <= c_i}`, and `Pi_Ci` denote Euclidean projection onto that disk. The joint equations are:

```text
r_q = grad E(q) + G^T nu + sum_i J_i^T lambda_i = 0
r_g = g(q) = 0
r_f,i = lambda_i - Pi_Ci(lambda_i + rho_i s_i(q)) = 0
```

Here `G = partial g / partial q`, and `nu` contains ideal-connection multipliers. Elastic connection potentials are included in `E`. Systems with only free bodies and friction omit the closure rows and multipliers.

The positive algorithm parameter `rho_i` has units N/m. It controls numerical scaling; the converged friction law is independent of its value. A zero-radius disk represents zero tangential force.

Semismooth Newton solves these primal–dual equations using generalized projection derivatives and consistent mechanical derivatives. Rotations use the local updates established in ADR-0009. Fixed normal loads and tangent geometry give a well-defined nonsmooth objective; nonlinear rotational geometry still requires globalization and convergence checks.

### Reference method and outer updates

Small-contact active-set cases provide an independently checkable reference. Sticking points impose zero tangential displacement, with friction forces obtained from the constrained balance equations. Force limits and sliding-direction consistency validate the resulting classification. The reference and main solvers share the same mechanical model and friction law.

An outer iteration updates normal loads and dissipation contact geometry from the current solution. Inner solves keep these quantities fixed, while conservative contact is reevaluated at trial states. The outer solve handles newly formed and released contacts. Sample identity and tangent-coordinate consistency support warm starts.

### Projection derivatives and Newton system

For one contact, set `y = lambda + rho s` and `u = y / ||y||`. With positive disk radius `c`, the projection derivative `D` is:

```text
||y|| < c: D = identity
||y|| > c: D = (c / ||y||) (identity - u u^T)
||y|| = c: D_gamma = identity - gamma u u^T, 0 <= gamma <= 1
c = 0:    D = 0
```

The boundary family is the Clarke generalized Jacobian. Strict interior and exterior points use their exact branch derivatives. Boundary ties use the branch of the last accepted state; new contacts start with the interior branch. A failed linear model or line search can retry the other boundary branch. Floating-point tie tolerances apply only to branch selection. Projection evaluation, residuals and final force limits retain their original definitions. Diagnostics record branch choices and retries.

For one contact and ideal connections, the generalized Newton matrix has blocks:

```text
[ H_L       G^T   J^T        ] [delta_q     ] = -[r_q]
[ G         0     0          ] [delta_nu    ]    [r_g]
[-rho D J   0     identity-D ] [delta_lambda]    [r_f]
```

Here `H_L = Hessian(E) + sum_j nu_j Hessian(g_j) + sum_i,alpha lambda_i,alpha Hessian(s_i,alpha)`. Kinematic curvature is included when the primary state uses reduced coordinates. Multiple contacts contribute stacked friction rows and dual columns. A sticking interior branch recovers `J delta_q = -s`. The full matrix is treated as a general primal–dual system.

### Residual line search and safeguards

Let `F = (r_q, r_g, r_f)`, `z = (q, nu, lambda)` and `Fhat = W F`. The fixed row scaling `W` makes force, torque, closure-position, closure-orientation and friction residuals comparable. Unknown scaling accompanies rank detection and minimum-norm solves. Scales remain fixed throughout an inner solve.

The line-search merit is `Psi(z) = 0.5 ||Fhat(z)||²`. Free translation, local rotation, joint-coordinate and multiplier corrections share one trial factor `alpha`. Free rotations use `exp(skew(alpha delta_theta)) R`; joint updates use their defined manifold operations, followed by forward kinematics. Trial dual forces may lie outside their disks. Every trial reevaluates the original projection and full mechanical residual.

For an accurately solved Newton equation, the reference sufficient-decrease test is:

```text
Psi(z_trial) <= Psi(z) - sigma alpha ||Fhat(z)||²
```

Backtracking starts at a full step. The illustrative values `sigma = 1e-4` and shrink factor `0.5` are experimental candidates. Boundary choices, inexact solves and rank deficiency can prevent the selected direction from passing this test; the criterion is an acceptance rule with explicit failure reporting.

The safeguard sequence checks the scaled linear residual, retries eligible boundary derivative choices, then permits a trust-region or damped residual-minimization direction. Safeguard directions use acceptance tests appropriate to their predicted reduction and verify the actual original residual. Exhausting the configured retry budget reports failure and preserves the previous accepted simulation state. Algebraic damping affects the search direction; final acceptance evaluates the original friction law.

The unconstrained frictionless smooth reference retains objective-based Newton and Armijo backtracking. Hard connections and friction use the joint residual merit specified here.

### Redundant contacts and force distribution

All contact samples and friction disks remain represented. A flat cuboid with four bottom samples has eight tangential constraint rows but only three independent first-order tangential combinations. Repeated constraints and multiple admissible force distributions can make the Newton system rank deficient.

The small CPU reference implementation uses rank-revealing QR or SVD. Consistent linear systems admit a scaled minimum-norm correction; inconsistent linear models use a least-squares correction followed by actual residual checks. Rank thresholds and linear residuals are logged. Constraint reduction, if used internally, preserves the full original residual and every sample's friction bound for acceptance.

A particular force distribution exceeding one disk does not establish that the body's sticking state is infeasible. The coupled solve permits force redistribution. Minimum-norm selection chooses a numerical correction; complete projection residuals establish friction admissibility. A singular linear system alone carries no physical sticking/sliding conclusion.

Stable sample identifiers and consistent tangent bases support force warm starts. An optional auxiliary force-selection problem may minimize deviation from history while preserving resultant force, resultant torque and all friction disks. Its implementation is subsequent work. Any numerical perturbation is recorded together with its effect on residuals and accumulated drift.

### Outer consistency and final acceptance

Outer iterations update normal loads and dissipative contact geometry. Inner solves freeze these data while reevaluating conservative contact at trial states. New and released contacts are handled at outer boundaries; zero normal load gives a zero-radius friction disk.

Final acceptance jointly checks scaled mechanical residuals, hard-connection position/orientation errors, projection residuals, sticking displacement, disk admissibility and outer load/contact consistency. The updated loads are also used to reevaluate the frozen-parameter mechanical residual at the candidate state. Small steps alone indicate progress or stagnation; the stated residual and feasibility conditions establish convergence.

Statuses distinguish convergence, iteration limits, stagnation, line-search failure and numerical failure. Only converged steps are automatically committed. Numeric tolerances, scaling formulas, outer relaxation and iteration budgets remain subsequent specifications.

## Consequences

The hybrid organization reduces tree-mechanism variables and enforces tree geometry through forward kinematics. Hard closures add multipliers and rank-sensitive saddle-point structure. Kinematic second derivatives, consistent initialization and joint-space diagnostics add engineering work. Maximal-coordinate reference cases support validation of the shared model.

The CPU research model combines Backward-Euler-type rigid inertia, compliant normal contact and nonsmooth Coulomb friction. Static friction supports load at zero discrete slip; implementation acceptance measures residual slip and sustained-load drift. Primal–dual unknowns, generalized derivatives and rank-aware linear algebra add implementation cost.

Semismooth Newton is the main friction solver. Small-contact active-set cases provide independent reference solutions; proximal projection iteration and velocity-regularized friction remain optional comparisons. The latter's low-speed creep lies outside accepted static-friction behavior.

The accepted scope is mathematical architecture and the solver policies stated above. Initial angular-velocity construction, normal damping, surface quadrature, normal-contact smoothing, production numerical parameters and higher-order integrators remain subsequent specifications. Parameter values from examples serve as experimental candidates. Implementation and physical-error studies remain subsequent work.

## Validation

Validation status: Pending implementation and experiments.

- Check smooth local gradients, Hessian-vector products and mechanical Jacobian-vector products with directional finite differences.
- Compare maximal-coordinate references and reduced-coordinate pendulum/slider cases; verify forward kinematics, local Jacobians and Hessians.
- Validate fixed and floating roots, branching trees, four-bar and slider-crank closure, and articulated-body contact with static friction. Track closure errors, rank changes and consistent state recovery at failed steps.
- Check normal force, force moment and translation–rotation coupling; cover symmetric support and tilted contact.
- Verify disk projections and branch derivatives, including boundary ties, zero-radius disks and derivative retry behavior.
- Compare active-set references and semismooth Newton for pushes below, at and above the friction limit, inclined-plane sticking, load reversal and contact release.
- Cover one-, two- and four-point support. Check rank diagnostics, force redistribution, all friction bounds, resultant force/torque and accumulated sticking drift.
- Verify line-search residual decrease, outer consistency and explicit failure handling. Record ranks, linear residuals, branch choices and iteration counts.
- Compare free principal-axis rotation, general free rotation and plane contact across time steps and tolerances using the documented Backward Euler baseline. Group related cases as parameterized tests to control test-file count.

## References

- [SuperDex rigid actors](https://projectsuperdex.com/physics/docs/concepts/actors/rigid_actors/)
- [SuperDex dynamics](https://projectsuperdex.com/physics/docs/concepts/dynamics/)
- [SuperDex contact](https://projectsuperdex.com/physics/docs/concepts/contact/)
- [SuperDex solvers](https://projectsuperdex.com/physics/docs/concepts/solvers/)
- [Siconos friction-contact formulations and solvers](https://nonsmooth.gricad-pages.univ-grenoble-alpes.fr/siconos/users_guide/problems_and_solvers/friction_contact.html)


- [SuperDex articulated actors](https://projectsuperdex.com/physics/docs/concepts/actors/articulated_actors/)
- [Newton articulation coordinates and loop closure](https://newton-physics.github.io/newton/latest/concepts/articulations.html)
