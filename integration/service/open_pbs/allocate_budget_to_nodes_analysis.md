# Analysis of `allocate_budget_to_nodes`

## The core problem

Each node $i$ has its own quadratic model:

$$S_i(P) = A_i(x_{0,i} - P/P_{max})^2 + B_i(x_{0,i} - P/P_{max}) + C_i$$

The function needs to find power allocations $P_1, P_2, \ldots, P_N$ such that:

1. **Equal slowdown**: $S_1 = S_2 = \cdots = S_N = S^\ast$
2. **Budget constraint**: $\sum P_i = \text{budget}$

## Why not just solve it analytically?

For a given target slowdown $S^\ast$, you *can* invert each node's quadratic to get $P_i(S^\ast)$ — that's what `power_at_slowdown` does via the quadratic formula. But the budget constraint requires:

$$\sum_{i=1}^{N} P_i(S^\ast) \cdot P_{max} = \text{budget}$$

Each $P_i(S^\ast)$ contains a square root term: $x_{0,i} - \frac{-B_i + \sqrt{B_i^2 - 4A_i(C_i - S^\ast)}}{2A_i}$

A **sum of square roots** with different coefficients under each radical has no closed-form inverse. You can't algebraically solve for $S^\ast$. So the problem reduces to 1D root-finding on:

$$f(S^\ast) = P_{max} \cdot \sum P_i(S^\ast) - \text{budget} = 0$$

That's exactly what `power_deficit_at_slowdown` computes, and `bisect_power_deficit_by_slowdown` finds its zero crossing.

## Why the "long pole" bounds?

Bisection needs a valid bracket $[S_{lo}, S_{hi}]$ where the deficit changes sign. The bounds come from physical constraints:

- **`long_pole_slowdown_at_max_power`**: Even at full power ($P = P_{max}$), the *worst* node still has some nonzero slowdown due to its model coefficients. You can't target a slowdown below this — it's physically unachievable for that node. This is $S_{lo}$.

- **`long_pole_slowdown_at_min_power`**: The model's predicted slowdown for the worst node when evaluated at zero power ($P = 0$). This is not a real operating point — no node actually runs at zero watts. It is used purely as a **safe upper bound** ($S_{hi}$) for bisection. At this extreme slowdown, every node's required power is near zero, so total power is guaranteed to be well under any positive budget. This ensures the power deficit is negative at $S_{hi}$, which together with the positive deficit at $S_{lo}$ guarantees a sign change and therefore a root within the bracket.

The "long pole" terminology comes from the node that constrains the whole job — even if 99 nodes could run at 0% slowdown, one bottleneck node with worse coefficients forces the target slowdown up. The first bound ($S_{lo}$) has direct physical meaning: it's the best achievable equal slowdown. The second bound ($S_{hi}$) is a mathematical convenience — any value high enough to guarantee an under-budget allocation would work, and zero power is the most conservative choice.

## The budget-exceeds-balanced-power shortcut

If the budget is large enough to give every node the power it needs at the long-pole minimum slowdown, bisection is skipped. The best achievable equal-slowdown point is already at max power for all nodes, and bisecting further can't improve it.

## The slack redistribution at the end

After bisection finds the equal-slowdown allocation, the budget may not be fully consumed (due to clipping at $[0, P_{max}]$ or the bisection tolerance). The leftover watts are distributed proportionally to each node's remaining headroom ($P_{max} - P_i$), so nodes with more room to benefit get more of the surplus.

## Removing slack redistribution for bulk-synchronous workloads

Lines 148–153 perform the slack redistribution: after bisection finds the
equal-slowdown allocation, leftover budget (from bisection tolerance or
clipping to $[0, P_{max}]$) is distributed proportionally to each node's
remaining power headroom ($P_{max} - P_i$). This deliberately **breaks** the
equal-slowdown balance because nodes with more headroom receive more extra
power and thus run faster than the rest.

If lines 148–153 are removed, the function instead returns the pure
equal-slowdown allocation from line 146:

```python
power_by_node = [p * max_node_power for p in power_at_slowdown(slowdown, x0, A, B, C)]
```

For **bulk-synchronous parallel (BSP)** applications, this is preferable for
two reasons:

### 1. Uniform performance (no wasted computation at barriers)

In a BSP application, all nodes must synchronize at barriers between phases.
Job throughput is determined by the **slowest node** in each phase. The
bisection result already targets $S_1 = S_2 = \cdots = S_N = S^\ast$, meaning all
nodes are expected to arrive at each barrier at the same time.

The slack redistribution gives some nodes more power than they need for $S^\ast$.
Those nodes finish their phase earlier — but then idle at the barrier waiting
for the others. The extra computation speed is invisible to overall job
performance because the barrier serializes progress.

Without redistribution, all nodes are paced equally. No node finishes early and
no node holds up the group. The wall-clock time per phase is identical with or
without the slack redistribution, because the target slowdown $S^\ast$ (which
determines the long-pole) is unchanged either way.

### 2. Energy savings (unused budget is not consumed)

The redistributed power (`unused_budget` watts) is consumed by nodes that run
ahead of the barrier and then idle. This idle waiting still draws power but
contributes zero useful work. During a barrier stall, the fast nodes are
spending energy doing nothing productive.

By not redistributing the slack, those watts are simply never allocated. The
total power draw of the job drops by `unused_budget` watts, and that energy is
saved with no impact on job completion time.

### Quantifying the savings

The energy saved equals the slack budget times the job runtime:

$$E_{saved} = (\text{budget} - \sum P_i^{equal}) \times T_{job}$$

where $P_i^{equal}$ is the equal-slowdown allocation from bisection. The slack
arises from:
- **Bisection tolerance** (0.1 W per the code): typically negligible.
- **Clipping**: if bisection targets a slowdown where some node's required
  power exceeds $P_{max}$ or falls below 0, `clip_list` caps it, leaving that
  node's share partially unspent relative to the budget equation.
- **Budget overshoot shortcut** (lines 133–137): when the budget exceeds what
  equal-slowdown requires, the entire excess becomes slack.

The third case — budget exceeding balanced power — can produce the largest
slack and therefore the largest energy savings from removing redistribution.

## Summary

| Aspect | Why it's needed |
|---|---|
| Bisection | Sum-of-square-roots has no closed-form inverse |
| Long pole bounds | Bracket the search to physically achievable slowdowns |
| Budget check shortcut | Avoids bisection when budget already exceeds what equal-slowdown requires |
| Slack redistribution | Fully uses the budget after clipping and tolerance effects |

If all nodes had **identical** models, you'd just divide the budget by $N$ and be done. The complexity is entirely a consequence of heterogeneous per-node coefficients making the equal-slowdown allocation a non-trivial numerical problem.

For BSP workloads, the slack redistribution (lines 148–153) is counterproductive: it spends extra energy to make some nodes faster, but that speed is wasted at synchronization barriers. Removing it preserves the equal-slowdown property and saves the corresponding energy with no impact on job completion time.
