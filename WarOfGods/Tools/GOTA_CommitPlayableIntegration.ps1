# AAA-style commits for Playable Integration work on feature/gota-cinematic-frontend.
# Run from repo root: powershell -ExecutionPolicy Bypass -File WarOfGods\Tools\GOTA_CommitPlayableIntegration.ps1
# Does NOT push. Does NOT rewrite history. Leaves Quixel BP noise unstaged.

$ErrorActionPreference = 'Stop'
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $Repo

function Commit-IfStaged([string]$Message) {
    $staged = git diff --cached --name-only
    if (-not $staged) {
        Write-Host "SKIP (nothing staged): $Message" -ForegroundColor Yellow
        return
    }
    git commit -m $Message
    if ($LASTEXITCODE -ne 0) { throw "commit failed: $Message" }
    Write-Host "OK $Message" -ForegroundColor Green
}

Write-Host "Repo: $Repo" -ForegroundColor Cyan
Write-Host "Commits on HEAD: $(git rev-list --count HEAD)"

# --- 1. gitignore ---
git add .gitignore
Commit-IfStaged @"
chore: ignore Marketplace vault packs in Content

Keep Fab/Megascans libraries on disk for arena dress without flooding git. Track Source, Docs, Tools, and curated dress DataAssets only.
"@

# --- 2. combat timing ---
git add -- `
  WarOfGods/Source/WarOfGods/Public/Combat/CombatTimingComponent.h `
  WarOfGods/Source/WarOfGods/Public/Combat/CombatTimingConfig.h `
  WarOfGods/Source/WarOfGods/Public/Combat/CombatTimingTypes.h `
  WarOfGods/Source/WarOfGods/Private/Combat/CombatTimingComponent.cpp `
  WarOfGods/Source/WarOfGods/Private/Combat/CombatTimingConfig.cpp `
  WarOfGods/Source/WarOfGods/Private/Combat/CombatTimingConsoleCommands.cpp `
  WarOfGods/Source/WarOfGods/Public/Combat/CombatBootstrapSubsystem.h `
  WarOfGods/Source/WarOfGods/Private/Combat/CombatBootstrapSubsystem.cpp `
  WarOfGods/Docs/GOTA_IMPL_04_06_COMBAT_TIMING.md `
  WarOfGods/Docs/GOTA_COMBAT_DESIGN.md `
  WarOfGods/Docs/GOTA_COMBAT_AUTHORITY_CONTRACT.md `
  WarOfGods/Docs/ADR_GAS_ADOPTION.md
Commit-IfStaged @"
feat(combat): add perfect parry, dodge, and divine counter timing

Land C++ timing contracts so Blueprint combat can emit verified events without duplicating damage authority.
"@

# --- 3. story ---
git add -- `
  WarOfGods/Source/WarOfGods/Public/Story `
  WarOfGods/Source/WarOfGods/Private/Story `
  WarOfGods/Content/Story `
  WarOfGods/Content/__ExternalActors__/Story `
  WarOfGods/Content/__ExternalObjects__/Story `
  WarOfGods/Docs/GOTA_STORY_BIBLE.md `
  WarOfGods/Docs/GOTA_STORY_DESIGN.md `
  WarOfGods/Docs/GOTA_STORY_NEXT_PACKAGES.md `
  WarOfGods/Docs/GOTA_STORY_PROLOGUE.md `
  WarOfGods/Source/WarOfGods/Public/UI/Widgets/GOTAStoryMenuWidget.h `
  WarOfGods/Source/WarOfGods/Private/UI/Widgets/GOTAStoryMenuWidget.cpp
Commit-IfStaged @"
feat(story): ship Fallen Sanctuary vertical-slice C++ and prologue map

Add story GameMode/GS, objectives/save hooks, and L_GOTA_Story_Prologue for Chapter 1 playtests.
"@

# --- 4. horde ---
git add -- `
  WarOfGods/Source/WarOfGods/Public/Arenas/Horde `
  WarOfGods/Source/WarOfGods/Private/Arenas/Horde `
  WarOfGods/Source/WarOfGods/Public/UI/Widgets/GOTAHordeHUDWidget.h `
  WarOfGods/Source/WarOfGods/Public/UI/Widgets/GOTAHordeResultsWidget.h `
  WarOfGods/Source/WarOfGods/Private/UI/Widgets/GOTAHordeHUDWidget.cpp `
  WarOfGods/Source/WarOfGods/Private/UI/Widgets/GOTAHordeResultsWidget.cpp `
  WarOfGods/Docs/GOTA_HORDE_MODE_DESIGN.md `
  WarOfGods/Docs/GOTA_HORDE_TEST_RESULTS.md `
  WarOfGods/Content/Arenas/Horde
Commit-IfStaged @"
feat(horde): harden Wrath of the Gods match flow and HUD bases

Extend wave director, runtime match definitions, and Horde HUD/results widget bases for Titan's Quarry PIE.
"@

# --- 5. duel ---
git add -- `
  WarOfGods/Source/WarOfGods/Public/Arenas/Duel `
  WarOfGods/Source/WarOfGods/Private/Arenas/Duel `
  WarOfGods/Source/WarOfGods/Public/UI/Widgets/GOTADuelHUDWidget.h `
  WarOfGods/Source/WarOfGods/Public/UI/Widgets/GOTADuelResultsWidget.h `
  WarOfGods/Source/WarOfGods/Private/UI/Widgets/GOTADuelHUDWidget.cpp `
  WarOfGods/Source/WarOfGods/Private/UI/Widgets/GOTADuelResultsWidget.cpp `
  WarOfGods/Docs/GOTA_PVP_ARCHITECTURE.md `
  WarOfGods/Docs/GOTA_PVP_DESIGN.md `
  WarOfGods/Docs/GOTA_PVP_TEST_RESULTS.md
Commit-IfStaged @"
feat(pvp): complete Trial of Champions 1v1 match lifecycle

Add duel rules DA hooks, death/timeout bridges, and HUD/results widget bases. BP NotifyDied/ObserveHealth still required for DoD.
"@

# --- 6. arenas dress ---
git add -- `
  WarOfGods/Source/WarOfGods/Public/Arenas/ArenaVisualDressDefinition.h `
  WarOfGods/Source/WarOfGods/Public/Arenas/ArenaVisualDressDefaults.h `
  WarOfGods/Source/WarOfGods/Public/Arenas/ArenaStreamingAnchor.h `
  WarOfGods/Source/WarOfGods/Public/Arenas/ArenaGrayboxBuilder.h `
  WarOfGods/Source/WarOfGods/Private/Arenas/ArenaVisualDressDefaults.cpp `
  WarOfGods/Source/WarOfGods/Private/Arenas/ArenaStreamingAnchor.cpp `
  WarOfGods/Source/WarOfGods/Private/Arenas/ArenaGrayboxBuilder.cpp `
  WarOfGods/Source/WarOfGods/Private/Arenas/ArenaGameMode.cpp `
  WarOfGods/Source/WarOfGods/Private/Arenas/ArenaConsoleCommands.cpp `
  WarOfGods/Content/Arenas/Data/DA_Dress_TitansQuarry.uasset `
  WarOfGods/Content/Arenas/Data/DA_Dress_AshfallCrucible.uasset `
  WarOfGods/Content/Arenas/Data/DA_Dress_FrostboundCitadel.uasset `
  WarOfGods/Content/Arenas/Data/DA_Dress_VerdantSanctuary.uasset `
  WarOfGods/Content/Arenas/Data/DA_Dress_SunscarExpanse.uasset `
  WarOfGods/Content/Arenas/Data/DA_Dress_StormbreakCoast.uasset `
  WarOfGods/Content/Arenas/Data/DA_Dress_ShatteredDominion.uasset `
  WarOfGods/Content/Arenas/Data/DA_Dress_FallenSanctuary.uasset `
  WarOfGods/Config/DefaultGame.ini `
  WarOfGods/Tools `
  WarOfGods/Docs/GOTA_ARENA_ASSET_DRESS.md `
  WarOfGods/Docs/GOTA_LOCAL_ASSET_MIGRATION.md `
  WarOfGods/Docs/GOTA_QUARRY_PILOT_MANIFEST.json `
  WarOfGods/Docs/GOTA_PLAYABLE_INTEGRATION_RESULTS.md
Commit-IfStaged @"
feat(arenas): data-driven visual dress for all seven arenas

Add soft-ref dress DAs, graybox integration, vault install tools, and ValidateArenaDress without replacing gameplay collision.
"@

# --- 7. frontend / mp / module plumbing ---
git add -- `
  WarOfGods/Source/WarOfGods/Public/UI/GOTAMenuCameraDirector.h `
  WarOfGods/Source/WarOfGods/Private/UI/GOTAMenuCameraDirector.cpp `
  WarOfGods/Source/WarOfGods/Private/UI/GOTAMenuSubsystem.cpp `
  WarOfGods/Source/WarOfGods/Private/Multiplayer/GotaSessionSubsystem.cpp `
  WarOfGods/Source/WarOfGods/WarOfGods.Build.cs `
  WarOfGods/WarOfGods.uproject `
  WarOfGods/Docs/GOTA_FRONTEND_MENU.md `
  WarOfGods/Docs/GOTA_MULTIPLAYER_FOUNDATION.md `
  WarOfGods/Docs/GOTA_IMPLEMENTATION_ROADMAP.md `
  WarOfGods/Docs/GOTA_IMPL_07_11_TRACK.md
Commit-IfStaged @"
feat(frontend): wire menu camera, sessions, and playable-integration roadmap

Keep cinematic menu C++ current and freeze new features behind validated Quarry/combat gates.
"@

Write-Host ""
Write-Host "=== Remaining (intentionally unstaged) ===" -ForegroundColor Cyan
git status -sb
Write-Host ""
Write-Host "=== Recent commits ===" -ForegroundColor Cyan
git log -12 --oneline
Write-Host "Total commits on HEAD: $(git rev-list --count HEAD)"
