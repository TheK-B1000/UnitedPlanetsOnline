# Implementations 07–11 — Track Status

**Updated with combat timing C++ (04–06).**

| Impl | Feature | Decision / status |
|------|---------|-------------------|
| **07** | GAS decision + migration | **Defer** — see `ADR_GAS_ADOPTION.md`. Revisit after networked combat + divine powers need prediction. No plugin enable this pass. |
| **08** | Online invites / matchmaking | **Later** — Null OSS + LAN only. `AreFriendInvitesSupported() == false`. Do not stub fake matchmaking. |
| **09** | Cinematic menu polish | **Partial C++** — `AGOTAMenuCameraDirector` intro delay + startup blend. Editor still owns WBP gold/stone dress + camera placement (`GOTA_FRONTEND_MENU.md`). |
| **10** | Giant arena streaming | **Scaffold** — `AArenaStreamingAnchor` markers (RegionId / EncounterId / soft level path / radius). No World Partition migration yet. |
| **11** | Selective asset integration | **Ongoing** — follow `GOTA_LOCAL_ASSET_MIGRATION.md` + Quarry pilot; do not bulk-commit Megascans. |

## Recommended next operator steps

1. Create `DA_GOTA_CombatTiming` and assign on pawns / defaults.  
2. Wire `ResolveIncomingAttack` in `BPC_DamageSystem` before damage.  
3. Bind Enhanced Input → `RequestParry` / `RequestDodge` / `RequestBlock` / `RequestDivineCounter`.  
4. Place cameras + intro delay on `L_GOTA_MainMenu`.  
5. Drop `AArenaStreamingAnchor` actors into Dominion graybox regions as labels.  
6. Keep inviting/matchmaking parked until Steam/EOS (or chosen OSS) is enabled.
