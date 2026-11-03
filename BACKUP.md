# Backup checklist (Timelapse / Optimierung work)

**Purpose:** Read-only safety checklist before any future firmware or application change.  
**Do not flash or install from this document alone.** Follow linked project guides on an idle, cold printer.

## Current baseline (Manuel / project context)

| Item | Documented value |
| --- | --- |
| Community firmware family | **V4.4.x** (user runs V4.4 family on ELEGOO base **02.01.01.07**) |
| CC2 Control on printer | **1.2.24** (user; mid-print — do not prompt to update now) |
| CC2 Control **1.2.25** | **Not** listed in this repository’s `CHANGELOG.md` or release docs at commit `01d99cd6f9d80881f44a4778b1622cb2d83fde13`; no evidenced timelapse/camera delta vs 1.2.24 here |
| This repo archive (Phase 0) | `/cursor/stores/self/docs/backups/centauri-carbon-2-community-2026-11-03-01d99cd6f9d80881f44a4778b1622cb2d83fde13.tar.gz` — SHA256 in `docs/backups/SHA256.txt` in the project store |

## Repository snapshot (Phase 0)

- **Branch:** `optimierung` (from `develop` at `01d99cd6f9d80881f44a4778b1622cb2d83fde13`)
- **Primary fork:** https://github.com/lpgamerzockt-ops/centauri-carbon-2-community  
- **Upstream reference:** https://github.com/Neme77/centauri-carbon-2-community  
- Tarball excludes `.git`; re-clone or `git checkout` the commit above for a full tree.

## Original ELEGOO firmware (same-version restore)

Documented in **this** repo — use before any flash attempt:

| Step | Source |
| --- | --- |
| Download original **02.01.01.07** package (external mirror, bytes match V4.4 builder pin) | `docs/FIRST_RUN.md:94–96`, `docs/INSTALL.md:19–21` |
| Expected SHA-256 | `278a0606a072ae432e9bf2d4f0157acba4570d1c85dbc768c359fef53ac5f406` (`docs/FIRST_RUN.md:96`, `docs/INSTALL.md:21`) |
| **Downgrade to older than 02.01.01.07 is blocked** by ELEGOO | `docs/FIRST_RUN.md:98`, `docs/INSTALL.md:23` |
| Install procedure | Same USB root **`.zip.sig`**, **Settings → Check for Updates → Offline Update** — `docs/FIRST_RUN.md:32–39`, OpenCentauri link `docs/FIRST_RUN.md:39` |
| If printer refuses package | **STOP** — record message; do not bypass checks (`docs/FIRST_RUN.md:100`, `docs/INSTALL.md:25`) |

## Neme77 Community Firmware restore

| Step | Source |
| --- | --- |
| Release artifacts + checksums | GitHub Releases — e.g. `docs/FIRST_RUN.md:11–28`, `docs/INSTALL.md:9–11` |
| USB offline update (signed `.zip.sig` unchanged on USB root) | `docs/FIRST_RUN.md:32–35`, `docs/INSTALL.md:9` |
| Return to original ELEGOO (same version) | `docs/FIRST_RUN.md:92–100` (also `docs/INSTALL.md:17–25`) |

**CC2 Control application rollback** (not full firmware): installer prints a backup path; restore with the exact `restore.sh` from that backup — `docs/UPDATE_CC2.md:37–38`, example command `docs/UPDATE_CC2.it.md:97–104` (English guide references the same workflow at `docs/UPDATE_CC2.md:37–38`). Restores the **application**, not prior ELEGOO firmware (`docs/UPDATE_CC2.it.md:106`).

**Portable CC2 Control backup (Settings → Backup and restore):** does **not** include full `autosave.cfg` or firmware — `docs/BACKUP_AND_LOCALE_PLAN.md:15–18`.

## `printer.cfg` / Klipper autosave backup (manual)

Not exported by CC2 Control’s JSON backup (`docs/BACKUP_AND_LOCALE_PLAN.md:17–18`). Before risky changes, copy from the printer over SSH while **idle and cold**:

| File | Role | Code reference |
| --- | --- | --- |
| `/opt/inst/printer.cfg` | Vendor base Klipper config | `cc2-control/src/bed_drying.h:28` |
| `/opt/inst/printer_dsp.cfg` | Config CC2 reads for pull-back / vendor sections | `cc2-control/src/canvas_unload.h:23`, `cc2-control/docs/API.md:121–122` |
| `/opt/usr/cfg/autosave.cfg` | SAVE_CONFIG overrides (meshes, Canvas, etc.) | `cc2-control/src/main.c:109`, `cc2-control/docs/API.md:121–122` |
| `/opt/usr/cfg/autosave_backup.cfg` | Written by firmware SAVE_CONFIG | `cc2-control/docs/API.md:140`, `cc2-control/docs/API.md:168` |

Example (adjust paths on your PC):

```sh
scp root@PRINTER-IP:/opt/inst/printer.cfg ./backup-printer.cfg
scp root@PRINTER-IP:/opt/inst/printer_dsp.cfg ./backup-printer_dsp.cfg
scp root@PRINTER-IP:/opt/usr/cfg/autosave.cfg ./backup-autosave.cfg
```

Store copies off-printer with a date stamp. Restoring autosave incorrectly can break meshes and calibration — only restore files you understand.

## Timelapse analysis deliverables (project store)

- Analysis: `/cursor/stores/self/docs/timelapse-analyse.md`  
- Proposal (not applied): `/cursor/stores/self/docs/timelapse-vorschlag.md`, `timelapse-vorschlag.patch`  
- German UI notes: `/cursor/stores/self/docs/deutsch-i18n-hinweise.md`

## Pre-flash STOP rule

If a step is **not** documented in the paths above (or linked release notes for your exact version), **STOP** before flashing. Do not invent restore URLs, checksums, or bypass procedures.
