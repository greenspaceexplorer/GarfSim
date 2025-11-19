# Garfield++ Simulation Guide

This document summarizes the structure of the HELIX Garfield++ repository, explains what each major script or executable does, and outlines how to run it. Use it together with `README.md`, which lists the minimal environment variables required for Garfield and DCSim.

---

## 1. Environment and Dependencies

1. Install ROOT and Garfield++ (the Dockerfile shows the expected versions).  
2. Export the environment variables referenced in `README.md` or simply run `source Garfield/setup_env.sh`. That script loads ROOT, the Garfield setup script, and exports `GARFIELD_IONDATA` and `DCsim_HOME`.
3. Most run scripts (`gen*.csh`) are written for `tcsh`; launch them with `tcsh scriptname.csh`. Bash driver scripts are limited to the HEAT/isochrone tools.
4. The Docker image (`Dockerfile`) builds Garfield++ into `/opt/garfieldpp`, exposes ROOT, and leaves you in `/opt`; copy this repo in and build targets from there if you prefer containers.

> **Reminder:** all binaries expect to be run from `GarfSim/Garfield` so that relative paths to `.gas` tables, ROOT style files, and helper assets resolve correctly.

---

## 2. Building the Code

### 2.1 DCSim Library

`DCSim/` contains the custom digitization/noise library (`DCsim.hh`, `DCsim.cc`). Build it first:

```bash
cd DCSim
make
```

This produces `Library/libDCsim.a`, which the Garfield executables link against (see `Garfield/makefile`).

### 2.2 Garfield Executables

All Garfield-level programs live beside the `makefile` in `Garfield/`. Each source file already has a corresponding target (see the sprawling list in `Garfield/makefile`). Build any tool with:

```bash
cd Garfield
make <target-name>    # e.g. make smalljet
```

You can invoke ROOT macros directly as well: `root -l -b -q macro.C+`.

---

## 3. Shared Assets and Helper Files

- **`Garfield/rootlogon.C`, `crestrootstyle.C`, `attrib.C`** – style helpers automatically loaded when ROOT starts; keep them next to the binaries so plots adopt the same look (CREST style, legend formatting, etc.).
- **Gas tables** (multiple `.gas` files) – Magboltz lookup tables used by most simulations (`co2_90_AR_10_T273.gas`, `Flight2024_*.gas`, etc.). Update them with the MakeGas utilities described later.
- **Reference data** – `Garfield/dcs.txt` (cross-section tables) and sample ROOT files (`NoiseStudy.root`, `FieldPlots.root`, `signalBe*.root`, etc.) serve as seeds for tests and filtering pipelines.

---

## 4. Detector Event Simulators

### 4.1 `smalljet` Family

Build: `make smalljet smalljet_drifttime smalljet_drifttime_electron_only ...`

Purpose:
- `smalljet` (`smalljet.C`) – full HELIX small-jet drift chamber simulation with ionization, drift, avalanche, and digitization.
- `smalljet_drifttime*` variants – focus on drift-time studies (with or without coordinates/electrons only).
- `smalljet_fieldplots` – writes ROOT file (`FieldPlots.root`) containing the E-field topology for quick inspection.
- `smalljet_check_num_e` – quick diagnostic counting primary electrons per track.

Run workflow:
1. Set the required environment variables (`DCSimNtrack`, `DCSimtrackx`, `DCSimtrackang`, `DCSimOutFile`, etc.). The `gen*.csh` scripts in this folder already do this (see §5).  
2. Execute `./smalljet` (or the desired variant). Add `-b` to suppress interactive canvases when batching.
3. Collect the produced ROOT files (usually `*.root` containing wire waveforms and histograms).

### 4.2 `jet`, `ion_signal`

Legacy demonstration programs (Garfield examples for TPC/jet chambers). Build with `make jet` / `make ion_signal` and run directly to visualize drift lines and signals.

### 4.3 `field_map_gen`

Sweeps track angles/positions and records field properties in ROOT/CSV outputs. Export the same env vars used by `smalljet`, then run:

```bash
DCSimNtrack=100 DCSimtrackx=6 DCSimtrackang=0 ./field_map_gen
```

Expect longer runtimes; the code enables OpenMP parallelism if available.

### 4.4 `full_tpc_sim`

Models a full IROC/OROC slice with optional gating. Build with `make full_tpc_sim` and run `./full_tpc_sim`. Edit the top of `full_tpc_sim.C` to toggle IROC/OROC, gating state, or voltages before rebuilding.

---

## 5. Batch/Launch Scripts

Located in `Garfield/`:

| Script | Description | How to run |
| --- | --- | --- |
| `genTracks.csh`, `genTracks2.csh`, `genTracks3.csh` | Fire multiple `smalljet` runs with pre-set track positions and CPU affinity (via `taskset`). | `tcsh genTracks.csh` (edit file to adjust starting positions or CPU masks). |
| `genAngles.csh`, `genAnglesLarge.csh` | Sweep track angles at fixed positions, calling `./smalljet`. The “Large” variant runs only high angles (`0.7–1.0 rad`). | `tcsh genAngles.csh` |
| `genNormal.csh`, `genKeith.csh`, `gen_notracks*.csh` | Convenience wrappers to produce standard samples (normal incidence, Keith’s settings, no tracks but drift-time studies, etc.). | `tcsh genNormal.csh` |
| `genFilter.csh` | Iterates over generated ROOT files and launches `./filterjob` with matching output names. | `tcsh genFilter.csh` |
| `gen_env.sh` | Minimal environment template (exporting `DCSim*` vars); source it before running binaries manually. | `. ./gen_env.sh` |
| `print_env.csh` | Prints the current environment (debug tool). | `tcsh print_env.csh` |
| `HEATModel_keith.sh`, `HEATModel_planeAndX_exec.sh`, `TScaleIso_exec.sh` | Bash drivers for isochrone/HEAT sweeps, described in §7. | `bash HEATModel_keith.sh`, etc. |

All scripts assume you are already in `Garfield/` with the executables compiled.

---

## 6. Filtering and Digitization (`filterjob`)

- **Source:** `Garfield/filterjob.cc`
- **Build:** `make filterjob`
- **Inputs:** `DCSimRootfile` (raw signal from `smalljet`), `DCSimFilterRootfile` (output path), `DCSimtrackx`, `DCSimtrackang`, `DCSimNtrack`. The script also opens `rootfiles/Besignal*.root` to obtain precomputed single-hit templates.
- **Execution:** `./filterjob` (after exporting the env vars). The job uses `DCsim` to digitize waveforms, apply shaping filters (configurable constants near lines 70–90), add noise from `NoiseStudy.root`, and records threshold timestamps in a new ROOT tree.
- **Post-processing:** Inspect the output ROOT file with ROOT or combine them via the hadding utilities (`hadd_Efield_profiles*.C`, etc.).

The `genFilter.csh` script shows how to batch over multiple track positions/angles.

---

## 7. Isochrone and HEAT Tools

| Executable | Purpose | Invocation |
| --- | --- | --- |
| `IsoHEATB` | Extracts HEAT model field maps (from tar archives), builds Garfield grids, and exports isochrones along configurable planes/x slices. | `./IsoHEATB <Temp[K]> <col-step> <x-step> <column#> <x-slice[mm]>` (see comment at top of `IsoHEATB.C`). |
| `HEATModel_keith.sh` | Serializes `IsoHEATB` runs with throttled concurrency; edit to change arguments. | `bash HEATModel_keith.sh` |
| `HEATModel_planeAndX_exec.sh` | Wrapper to call `IsoHEATB` with `(temperature, x slice, column index)` passed as positional args. | `bash HEATModel_planeAndX_exec.sh 301 -150 1` |
| `TScaleIso_run` | Rescales existing isochrones vs temperature; outputs ROOT/ASCII dumps. | `./TScaleIso_run <Temp[K]> <unused> <scale>` |
| `TScaleIso_exec.sh` | Lightweight wrapper launching `TScaleIso_run $1 0.0 1.0`. | `bash TScaleIso_exec.sh 299` |

Supporting macros (`combine_isochrones_to_ROIlike.C`, `grabIsochrone*.C`, `filter_flight_isochrones*.C`, `HijackedIsochroneDCT*.C`, `ViewIsochroneDCT.C`) are ROOT scripts for merging/visualizing isochrone studies. Run as `root -l -b -q 'macro.C()'` after pointing them to the folder containing the isochrone ROOT files.

---

## 8. Gas Table Generation and Property Scans

### 8.1 Creating `.gas` Files

Executable | Notes | Sample command
---|---|---
`MakeGasFile`, `MakeGasFile_2`, `MakeGasFile_3` | Simple Magboltz table generators for fixed compositions. | `./MakeGasFile` (hardcoded mix inside each source). |
`MakeGasFile_flight` | Accepts CLI parameters (pressure psi, temperature K, B-field flag). | `./MakeGasFile_flight <psi> <temp[K]> <Bflag>` |
`MakeGasFile_multiB` | Sweeps multiple magnetic field strengths per call. | `./MakeGasFile_multiB` |
`MakeGasFineGridTemperature`, `_Bfield`, `_Bfieldoff` | Scans temperature ranges at fine granularity; outputs CSVs like `lorentz_vs_temp.csv`. Arguments: `<psi> <degC_low> <degC_high>`. | `./MakeGasFineGridTemperature 14.6 20 40` |
`GasFileGenerator` | Minimal example (Ar/CO₂ mix at 3 atm). | `./GasFileGenerator` |
`LoadGasFile`, `LoadGas_bfield_File`, `printgas` | Inspect existing `.gas` files; the `_bfield_` version prints drift velocity and Lorentz angle for each point; `printgas` also loads ion mobility tables. | `./printgas` |
`chambertest.C` (ROOT macro under `DCSim/`) | Another quick gas generator (CO₂ 95% / Ar 5%). | `root -l -b -q chambertest.C` |

All of these rely on Magboltz being available through Garfield (guaranteed if `setup_env.sh` was sourced).

### 8.2 CSV/Plotting Utilities

`Csv_drift_vs_gascomp.C`, `Csv_drift_vs_pressure*.C`, `Csv_drift_vs_Temp.C`, `Csv_driftv_vs_bfield.C`, `Csv_driftv_vs_efield.C`, `Csv_efield_vs_Temp.C`, `Csv_lorentz_vs_temp.C` iterate through `.gas` grids, compute mean velocities/Lorentz angles, and write CSVs. Build with `make <target>` and run `./Csv_drift_vs_gascomp` (outputs appear as `.csv` files in the working directory).

---

## 9. Drift, Gain, and Avalanche Studies

Executable | Purpose | Inputs / Run tips
---|---|---
`basic_drift_time_calculator`, `_MC` | Compute drift-time distributions vs temperature/pressure (optional Monte Carlo). CLI parameters noted near the top of the files (pressure psi, temperature K). |
`driftv_vsEField_plotter` | Plots drift velocity vs E field at a given z slice. Run `./driftv_vsEField_plotter <z_mm>` and inspect `fieldprofile_v_vs_E.root`. |
`driftField_plotter_forkenichi`, `eField_plotter*`, `Efield_full_column`, `VoltageMap_ICRC2025` | Field-visualization tools for different geometries/plots. They typically take CLI args for z slices or iterate over defaults; outputs are saved as ROOT files/PDFs in `Garfield/plots/`. |
`avalanch_gain_calc`, `_v2`, `gain_calculator`, `gas_gain_study` | Simulate avalanche gain, optionally vs temperature (`avalanch_drifttime_vs_temp*`). Provide z position or other parameters as CLI args (see code comments). |
`track_plotter`, `track_numelectrons`, `track_energy_deltaElectrons` | Analyze tracks produced by `smalljet`; pass the ROOT filename as the first argument. |
`track_resolution_calculator` | Scores timing/position resolution using the digitized outputs. |
`plotDriftVelocitye` | Quick look at electron drift velocities using `ViewMedium`. |

Each macro prints intermediate diagnostics to stdout; redirect to log files when running long scans: `./gain_calculator 500 > gain_500.log`.

---

## 10. Potential/E-field Mapping Utilities

- `make_potential_map`, `_wires_for_cathode`, `_original` – export text lists of wire positions/voltages (`*_wires.txt`) plus ROOT data for cross-checking. They reuse the `smalljet` geometry, so set `DCSim*` env vars before running.
- `field_map_gen`, `ViewBFieldExample`, `smalljet_fieldplots` – produce `FieldPlots.root` and PDF/GIF snapshots (examples stored under `Garfield/plots/`).
- Numerous GIF/PDF files already exist as references (`plots/*.pdf`, `*.gif`); regenerate by re-running the originating macro (file name usually matches).

---

## 11. Data, Archives, and Misc Utilities

- `Garfield/archive/` keeps historical versions of key files (older `smalljet.C`, `.gas` tables, example signals). Reference them if you need to reproduce past analyses.
- `Garfield/memory_usage_macro.cpp` is a ROOT helper: `root -l -b -q 'memory_usage_macro.cpp("file.root")'` prints object sizes for diagnosing large files.
- `Garfield/garfroot.C` starts a TRint session with the custom style preloaded (`./garfroot`).

---

## 12. Recommended Workflow

1. **Setup** – Source `setup_env.sh` (or use the Docker image) to configure ROOT/Garfield/DCSim paths.
2. **Build** – Run `make` in `DCSim/` once, then `make <targets>` inside `Garfield/` for the tools you need.
3. **Simulate** – Use the `gen*.csh` scripts to export `DCSim*` variables and spawn `smalljet`/variants; for single runs, set the env vars manually (`export DCSimNtrack=100`, etc.).
4. **Filter/Digitize** – Post-process raw ROOT files with `./filterjob` (scripted via `genFilter.csh`).
5. **Analyze** – Leverage the drift/gain/field plotting executables or CSV macros to produce plots and tables; use the isochrone/HEAT suite when studying temperature or plane variations.
6. **Extend** – When adding new detectors or studies, mimic the existing `makefile` pattern: add `<target>: <source>.C` entries and run `make <target>`.

Feel free to adapt or expand this guide as the repository evolves.
