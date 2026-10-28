# Trial of Champions — Test Results

**Date:** 2026-10-27  
**Scope:** Implementation 03A (1v1 only)

## Automated / Editor

| Test | Status | Notes |
|------|--------|-------|
| Duel C++ compile | Pending / in progress | Rebuild after this land |
| `gota.Duel` 2-player listen | **Not run** | Manual PIE required |
| Countdown locks move input | **Not run** | |
| Round elimination via Destroy | **Not run** | Needs BP NotifyDied |
| Best-of-3 match complete | **Not run** | |
| Round timeout + health compare | **Not run** | Needs ObserveHealth from BP |
| Simultaneous KO unscored | **Not run** | |
| Disconnect → opponent wins | **Not run** | |
| Rematch Server RPC | **Not run** | |
| Return to lobby | **Not run** | |
| Moderate latency / packet loss | **Not run** | UE Net Emulation |
| WBP_GOTA_DuelHUD / Results | **Missing assets** | C++ soft paths ready |

## Blocking dependencies (do not mark DoD complete)

1. **BP `NotifyDied` + server Destroy** on lethal — without this, rounds stall on ragdoll.
2. **`ObserveHealth` after damage** — timeout health compare and HUD bars stay stale.
3. **`IDuelFighterInterface` on BP_Player** — without it, round reset only mirrors Stats observe, not true BP health.
4. **Authoritative hit validation** (parry/block windows) — still BP-local; document as known unfairness under latency until Phase 04+.

## Honesty

Compile success ≠ Definition of Done. DoD requires a real host+client best-of-three with consistent server/client outcomes.
