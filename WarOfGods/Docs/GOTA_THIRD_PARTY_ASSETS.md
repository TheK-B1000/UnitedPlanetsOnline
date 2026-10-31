# Third-party content (not in Git)

Some content the game uses is Epic sample or Fab content in uncooked source form. The GitHub remote is **public**, and Epic's licence terms don't allow redistributing those source assets publicly. So they are installed from Epic's own channels and checked against a manifest instead of being committed. This is the same policy as the Paragon packs (see `.gitignore`).

The packaged game contains them in cooked form, which is allowed.

**Check an install:**

```
python WarOfGods/Tools/GOTA_ThirdPartyAssets.py verify
```

It prints `OK` per pack, or lists each missing or different file, and exits with code 1 if anything is wrong.

## Required packs

| Pack | Source | Installs to | Check |
|---|---|---|---|
| Windwalker Echo (visual assets only) | Epic sample "Windwalker Echo" (Fab / Epic Launcher, free) | `Content/EchoContent/Characters/Echo` (144 files, 1.27 GB) | SHA-256 manifest `Tools/ThirdPartyManifests/EchoContent.txt` |
| Paragon: Greystone | Fab, free | `Content/ParagonGreystone` | key files present |
| Paragon: Countess | Fab, free | `Content/ParagonCountess` | key files present |
| Paragon: Sevarog | Fab, free | `Content/ParagonSevarog` | key files present |
| Paragon: Rampage | Fab, free | `Content/ParagonRampage` | key files present; patched by step 3 below |

## Setting up a new machine

1. **Install the Paragon packs.** Add them to the project from Fab: Greystone, Countess, Sevarog and Rampage.
2. **Install Windwalker Echo.**
   1. Create the Windwalker Echo sample project anywhere. The scripts assume `B:\Unreal\WindwalkerEcho`; to use another path, edit `DEST_PROJECT` in the script.
   2. Run Unreal's Migrate unattended from inside that project:
      ```
      UnrealEditor-Cmd <WindwalkerEcho.uproject> -ExecutePythonScript=<repo>/WarOfGods/Tools/GOTA_MigrateEcho.py -unattended -nop4 -nosplash
      ```
   3. This copies the two Echo meshes and all their dependencies (144 packages) with their original paths. It never overwrites existing files.
3. **Patch the Rampage Blueprint** so it compiles and cooks under UE 5.8. This removes a dead UE4 VR node.
   ```
   UnrealEditor-Cmd WarOfGods/WarOfGods.uproject -ExecutePythonScript=WarOfGods/Tools/GOTA_FixThirdPartyBlueprints.py -unattended -nop4 -nosplash
   ```
4. **Verify the install:** run `python WarOfGods/Tools/GOTA_ThirdPartyAssets.py verify`. It should report `OK` for every pack.

Git-tracked files that reference this content:
- `Content/Heroes/Data/DA_Hero_UPAWarden.uasset`
- `Content/Heroes/Paragon/Rigs/IK_GOTA_Echo.uasset` and `RTG_GOTA_Manny_Echo.uasset`

They load cleanly once the packs are installed.

**If the packs are missing:** the editor reports missing references. A cook fails if the Rampage pack is missing or unpatched.

## Changing a pack

If you deliberately re-migrate or edit Echo content, re-record its manifest and commit the manifest:

```
python WarOfGods/Tools/GOTA_ThirdPartyAssets.py write EchoContent
```

## Private storage alternative

If the repository becomes **private**, the Echo content could be tracked with Git LFS instead:
- It is 1.27 GB, within GitHub Free's 10 GiB LFS storage.
- The largest file is 89 MB, under the 100 MB per-file limit for plain Git.
- Steps: add `WarOfGods/Content/EchoContent/** filter=lfs diff=lfs merge=lfs -text` to `.gitattributes`, then remove the ignore rule.

Don't do this while the remote is public.
