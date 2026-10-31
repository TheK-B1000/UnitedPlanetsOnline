"""Third-party content the game needs but that is NOT in Git (Epic sample / Fab packs, uncooked
source assets: the GitHub remote is public, so they are distributed through Epic's channels instead).

  python Tools/GOTA_ThirdPartyAssets.py verify          # check every required pack is installed and intact
  python Tools/GOTA_ThirdPartyAssets.py write <pack>    # (maintainers) re-record a pack's manifest

Manifests live in Tools/ThirdPartyManifests/<pack>.txt: one "<sha256>  <bytes>  <path>" line per file,
paths relative to WarOfGods/Content. Exit code 1 if anything required is missing or different.
Setup steps for each pack: Docs/GOTA_THIRD_PARTY_ASSETS.md.
"""
import hashlib
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTENT = os.path.join(ROOT, "Content")
MANIFESTS = os.path.join(ROOT, "Tools", "ThirdPartyManifests")

# pack name -> folder under Content. Only packs listed here are required by shipped content.
PACKS = {
    "EchoContent": "EchoContent",
}
# Fab packs installed as-is (and one locally patched Blueprint), checked for presence only.
REQUIRED_FILES = [
    "ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone.uasset",
    "ParagonCountess/Characters/Heroes/Countess/Meshes/SM_Countess.uasset",
    "ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog.uasset",
    "ParagonRampage/Characters/Heroes/Rampage/Meshes/Rampage.uasset",
    "ParagonRampage/Characters/Heroes/Rampage/RampagePlayerCharacter.uasset",
]


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def files_of(folder):
    base = os.path.join(CONTENT, folder)
    for d, _, names in os.walk(base):
        for n in names:
            full = os.path.join(d, n)
            yield os.path.relpath(full, CONTENT).replace("\\", "/"), full


def write(pack):
    os.makedirs(MANIFESTS, exist_ok=True)
    rows = sorted(files_of(PACKS[pack]))
    with open(os.path.join(MANIFESTS, pack + ".txt"), "w", newline="\n") as out:
        for rel, full in rows:
            out.write("%s  %d  %s\n" % (sha256(full), os.path.getsize(full), rel))
    print("%s: recorded %d files" % (pack, len(rows)))


def verify():
    bad = 0
    for pack in PACKS:
        mpath = os.path.join(MANIFESTS, pack + ".txt")
        if not os.path.exists(mpath):
            print("%s: NO MANIFEST (%s)" % (pack, mpath))
            bad += 1
            continue
        missing, changed, ok = [], [], 0
        for line in open(mpath):
            digest, size, rel = line.rstrip("\n").split("  ", 2)
            full = os.path.join(CONTENT, rel)
            if not os.path.exists(full):
                missing.append(rel)
            elif os.path.getsize(full) != int(size) or sha256(full) != digest:
                changed.append(rel)
            else:
                ok += 1
        state = "OK" if not missing and not changed else "FAIL"
        print("%s: %s  %d ok, %d missing, %d changed" % (pack, state, ok, len(missing), len(changed)))
        for rel in (missing + changed)[:20]:
            print("   ", rel)
        bad += len(missing) + len(changed)
    absent = [r for r in REQUIRED_FILES if not os.path.exists(os.path.join(CONTENT, r))]
    print("Fab packs: %s  %d of %d key files present" % ("FAIL" if absent else "OK", len(REQUIRED_FILES) - len(absent), len(REQUIRED_FILES)))
    for r in absent:
        print("    missing", r)
    bad += len(absent)
    if bad:
        print("See Docs/GOTA_THIRD_PARTY_ASSETS.md to install the missing content.")
    return 1 if bad else 0


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "write":
        write(sys.argv[2])
    else:
        sys.exit(verify())
