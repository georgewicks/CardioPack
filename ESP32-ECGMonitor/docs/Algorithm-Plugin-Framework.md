# Algorithm Plugin Framework — Design

Status: **Proposal (2026-11-07)** — not yet implemented.

## 1. Why this exists

As the project grows, more analysis will be layered on top of the captured ECG:
ST-segment analysis, beat classification, rhythm/arrhythmia rules, and eventually
AI/ML inference. These have three different sizing problems that are easy to
conflate:

| Problem | What actually limits us | Right answer |
|---|---|---|
| **Code size** | 2 MB flash, 1 MB app partition, ~13% free (~130 KB) | Load code from SD at runtime (ELF loader) |
| **Model/data size** | RAM; model weights, ring buffers | PSRAM + SD-card weight blobs |
| **Updateability** | build → flash cycle for every tweak | A stable plugin interface + registry |

This document defines a **plugin framework** so that any analysis algorithm —
now or in the future — plugs into one place with one contract, and can be
evolved from "compiled in" to "loaded from the SD card" without re-architecting.

The three layers are:

1. **Layer 1 — the plugin ABI** (a C interface + registry). Prerequisite for all.
2. **Layer 2 — static plugins** (each algorithm is a component behind the ABI).
3. **Layer 3 — dynamic loading** (Espressif `elf_loader`; `.so` files from SD).

We do **Layer 1 + 2 first** — they deliver 80% of the "easy to add algorithms"
value with zero new dependencies — and add **Layer 3 only when** we need
true load-without-reflash.

---

## 2. The plugin interface (Layer 1)

One header, `components/ecg_algos/include/ecg_algo.h`, defines everything.

```c
/* ecg_algo.h — stable ABI for ECG analysis plugins. */

#pragma once
#include <stdint.h>
#include <stddef.h>

/* A plugin processes one ECG channel's samples (volts). Results are
 * streamed per block; plugins keep their own state in `ctx`. */
typedef struct ecg_algo_ctx ecg_algo_ctx_t;

typedef struct {
    const char *name;      /* stable id, e.g. "st_segment", "beat_classifier" */
    uint32_t    version;   /* semantic-ish, e.g. 0x00010000 = 1.0.0           */
    const char *desc;

    /* Lifetime. */
    ecg_algo_ctx_t *(*create)(const void *cfg, size_t cfg_len);
    void            (*destroy)(ecg_algo_ctx_t *ctx);
    void            (*reset)(ecg_algo_ctx_t *ctx);

    /* Process `n` samples at `fs_hz`. `out`/`out_cap` is a caller-owned
     * buffer; the plugin fills it with results and returns bytes written
     * (0 if nothing to emit this block). */
    int (*process)(ecg_algo_ctx_t *ctx,
                   const float *ecg, int n, float fs_hz,
                   void *out, size_t out_cap);

    /* Optional: describe one output struct for tooling/downstream code. */
    const char *out_schema;
} ecg_algo_def_t;

/* Host calls this from exactly one place to run a configured algorithm. */
typedef struct {
    const ecg_algo_def_t *def;
    ecg_algo_ctx_t       *ctx;
} ecg_algo_instance_t;
```

Why this shape:

- **`process()` is block-based and stateless at the API level** — exactly how
  `pan_tompkins_process()` already works. State lives in `ctx`, so plugins
  survive across blocks (the ECG arrives 215-sample blocks today).
- **`fs_hz` is passed per call** — the actual rate drifts (we've measured
  230–313 Hz across runs). Plugins must not hard-code 300 Hz.
- **`out`/`out_cap` is opaque** — the host just forwards bytes to the logger;
  a plugin's output struct can evolve without changing the framework.
- **`version` and `out_schema`** give the host enough to reject incompatible
  plugins and for tooling to describe output.

---

## 3. The registry (Layer 1)

A plugin is *discovered*, not called directly. `components/ecg_algos/registry.c`
owns the list of available algorithms and a small dispatch API:

```c
/* registry.h */
typedef struct {
    const ecg_algo_def_t *def;
    void                 *module;   /* NULL for static; dlopen handle for .so */
} ecg_algo_entry_t;

int  ecg_algo_register(const ecg_algo_def_t *def, void *module);
int  ecg_algo_unregister(const char *name);
const ecg_algo_def_t *ecg_algo_find(const char *name);
int  ecg_algo_list(char *buf, size_t cap);          /* for console/log dump   */
```

Static plugins self-register at boot via constructor attribute (or an explicit
`ecg_algos_static_init()` called from `app_main`), so adding a new algorithm is
one `.c` file + one line in the component's `CMakeLists.txt`.

---

## 4. Static plugins (Layer 2)

Each algorithm is a subfolder of `components/ecg_algos/`:

```
components/ecg_algos/
  include/ecg_algo.h
  include/registry.h
  registry.c
  QRSDetect/            # wraps the existing Pan-Tompkins detector
    qrs_plugin.c
  STSegment/            # future
    st_plugin.c
  BeatClassify/         # future (TinyML)
    classify_plugin.c
```

Example — wrapping today's detector:

```c
/* qrs_plugin.c — adapt pan_tompkins_process() to the ecg_algo ABI. */
#include "ecg_algo.h"
#include "pan_tompkins.h"
#include <stdlib.h>

typedef struct { pt_context_t pt; } qrs_ctx_t;

static ecg_algo_ctx_t *qrs_create(const void *cfg, size_t n) {
    qrs_ctx_t *c = calloc(1, sizeof *c);
    pan_tompkins_init(&c->pt);
    return (ecg_algo_ctx_t *)c;
}
static void qrs_destroy(ecg_algo_ctx_t *p) { free(p); }
static void qrs_reset(ecg_algo_ctx_t *p) { pan_tompkins_init(&((qrs_ctx_t*)p)->pt); }

typedef struct { uint32_t idx; uint32_t rr_ms; } qrs_beat_t;   /* = pt_beat_t */

static int qrs_process(ecg_algo_ctx_t *p, const float *x, int n, float fs,
                       void *out, size_t cap) {
    (void)fs; /* Pan-Tompkins currently uses PT_FS_HZ; see §6 note */
    return pan_tompkins_process(&((qrs_ctx_t*)p)->pt, x, n, out, cap / sizeof(qrs_beat_t))
         * (int)sizeof(qrs_beat_t);
}

static const ecg_algo_def_t g_qrs = {
    .name = "qrs", .version = 0x00010000, .desc = "Pan-Tompkins QRS detector",
    .create = qrs_create, .destroy = qrs_destroy, .reset = qrs_reset,
    .process = qrs_process, .out_schema = "qrs_beat_t{idx,rrs_ms}",
};
static void __attribute__((constructor)) qrs_register(void) {
    ecg_algo_register(&g_qrs, NULL);
}
```

The existing [PushToECGProc](/home/gwicks/Cardiology_instrumentation/ESP32-ECGMonitor/components/QRSDetect/QRSDispatcher.c:37) call
site in `main.c` becomes a registry lookup + `process()` — the behavior is
identical, but now it's pluggable.

---

## 5. Future algorithms and how they slot in

### ST-segment analysis
- **Pure math on the beat windows** (baseline → detect J-point → measure
  ST at J+60/80 ms against the PR baseline). Code is a few KB; fits today.
- Needs the **R-peak indices** from the QRS plugin, so it registers as a
  *downstream* consumer. This is where the plugin ordering matters: add an
  explicit **pipeline** (ordered list of plugin instances) rather than a flat
  set. ST runs *after* QRS, consuming its output.

### Beat classification / ML
- Use **`espressif/esp-dsp`** (already a dependency) for FFT/matrix kernels,
  or **tflite-micro** for a small CNN.
- The **weights are data**, not code: store them as a blob on the SD card and
  load into **PSRAM** (enable `CONFIG_SPIRAM`; verify the module has PSRAM —
  see §8). No ELF loading is required for weights.
- The *inference code* is a static plugin (Layer 2). Only if you want to ship
  *new inference code* without reflashing do you need Layer 3.

---

## 6. Data-flow integration

Today:

```
ADC tasks → adc_queue → sd_writer_task
                          └─ PushToECGProc(block) → pan_tompkins per channel
```

Proposed:

```
ADC tasks → adc_queue → sd_writer_task
                          ├─ record to CSV/ISHNE (unchanged)
                          └─ ecg_algo_pipeline_run(block, fs)
                               ├─ [qrs]         → beats → [st_segment] → events
                               └─ [classifier]  → labels
                               └─ results → log to SD / UART
```

Rules:

- **Feed the pipeline the same `record.channels[2]` blocks** the writer sees,
  so detection and recording never diverge.
- **Pass the measured `fs`**, not a constant — track it from the record
  timestamps (we already compute real rate from `RECORDING STARTED/COMPLETE`).
- **One pipeline instance per lead** if you later go multi-lead; the ABI is
  per-channel by design.

---

## 7. Dynamic loading with `elf_loader` (Layer 3, deferred)

Espressif's [`espressif/elf_loader`](https://components.espressif.com/components/espressif/elf_loader)
(v2.1.0) supports **ESP32-S3** and `idf >= 4.4.3`, so it works with our
**IDF v6.0.2**. Two APIs are provided:

- `esp_elf.h` — load + relocate + run a whole ELF "app".
- `dlopen`/`dlsym` — load a **shared object** and resolve exported symbols
  (the one we want: `dlsym(handle, "ecg_algo_get_def")` → `ecg_algo_def_t`).

Integration sketch:

```yml
# main/idf_component.yml
dependencies:
  espressif/elf_loader: "2.*"
```

```c
/* at boot, after SD mount */
void *mod = dlopen("/sdcard/plugins/st_segment.so", 0);
if (mod) {
    const ecg_algo_def_t *(*get)(void) = dlsym(mod, "ecg_algo_get_def");
    if (get) ecg_algo_register(get(), mod);       /* mod => unload later     */
    /* fall back to the built-in static plugin when dlopen fails */
}
```

The host must **export its API to the module** using
`ESP_ELFSYM_EXPORT("dspi_fft_f32", ...)` etc., so a `.so` can call back into
the firmware's DSP/ECG utilities. The `.so` is built with the component's
shared-library build rules (see its `build_shared_library_example`).

### Caveats specific to us

- **No MMU / no sandbox** — a buggy plugin crashes the whole device.
- **C++ exceptions & RTTI are unsupported** by the loader.
- **PSRAM strongly preferred** on S3 for non-trivial modules
  (`ELF_LOADER_LOAD_PSRAM`); confirm the module has PSRAM first.
- **IDF v6 porting risk** — we already hit header/API churn porting
  `logic_analyzer`; expect the same touch-ups here.
- **Debugging** uses the loader's `elf_addr2line.py` for backtraces.

---

## 8. Hardware prerequisites to verify

| Item | Current state | Action |
|---|---|---|
| PSRAM on module | SOC-supported, **not enabled**; physical presence unknown | Read module silkscreen (N8R2/N8R8/N16R8 = has PSRAM) or probe; enable `CONFIG_SPIRAM` |
| Flash | **2 MB**, app partition ~13% free | Fine for Layer 1+2; Layer 3 code goes on SD anyway |
| SD card | mounted (FatFS) | Already the natural store for `.so` + model weights |
| `esp-dsp` | already a dependency | Foundation for ST + TinyML kernels |

---

## 9. Roadmap / decision gates

1. **Implement Layer 1 + 2** — `components/ecg_algos/` with the ABI, registry,
   and a `qrs` static plugin that wraps Pan-Tompkins. *No new dependencies.*
   ✅ Gate: firmware builds, `qrs` produces identical output to today.

2. **Add ST-segment plugin** — introduces the ordered-pipeline concept
   (QRS → ST). ✅ Gate: ST values land in the SD log.

3. **Enable PSRAM** (if hardware allows) and load a model-weight blob for the
   classifier plugin. ✅ Gate: classifier runs from PSRAM.

4. **Add `elf_loader` (Layer 3)** — only when we want SD-deployed, no-reflash
   updates. ✅ Gate: a `.so` built independently loads and registers.

Each gate is independently shippable; none blocks the others.
