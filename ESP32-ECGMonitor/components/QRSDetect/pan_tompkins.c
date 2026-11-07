/**
 * @file pan_tompkins.c
 * @brief Pan-Tompkins QRS detector implementation (streaming).
 *
 * The difference equations and decision rules follow Pan & Tompkins, "A
 * Real-Time QRS Detection Algorithm", IEEE TBME, 1985, adapted to a 300 Hz
 * sample rate (the measured per-lead rate; see pan_tompkins.h). Coefficients
 * were computed offline (see tools/). The decision stage uses adaptive
 * thresholds SPKI/NPKI with T-wave rejection.
 *
 * A 60 Hz mains notch filter is applied ahead of the bandpass. The high-pass
 * stage is 4th order (two cascaded 2nd-order sections) for strong rejection
 * of respiration baseline wander below ~1 Hz.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "pan_tompkins.h"

/* 60 Hz mains notch, 2nd-order IIR (scipy.signal.iirnotch, Fs=300 Hz, Q=10). */
#define NOTCH_B0  0.940809f
#define NOTCH_B1 -0.581452f
#define NOTCH_B2  0.940809f
#define NOTCH_A1 -0.581452f
#define NOTCH_A2  0.881619f

/* Butterworth 2nd-order low-pass @ 11 Hz, Fs = 300 Hz (scipy.signal.butter) */
#define LP_B0  0.011374f
#define LP_B1  0.022748f
#define LP_B2  0.011374f
#define LP_A1 -1.676462f
#define LP_A2  0.721958f

/* Butterworth 4th-order high-pass @ 5 Hz, Fs = 300 Hz, as two cascaded
 * 2nd-order sections (scipy.signal.butter(..., output='sos')). The 4th-order
 * response gives ~80 dB attenuation at 0.5 Hz vs ~40 dB for the old 2nd-order
 * filter, removing respiration baseline wander from the integrator output. */
#define HP_B0  0.872061f
#define HP_B1 -1.744121f
#define HP_B2  0.872061f
#define HP_A1 -1.813875f
#define HP_A2  0.823866f
#define HP2_B0 1.000000f
#define HP2_B1 -2.000000f
#define HP2_B2 1.000000f
#define HP2_A1 -1.912540f
#define HP2_A2 0.923074f

void pan_tompkins_init(pt_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}

static int cmp_float_asc(const void *pa, const void *pb)
{
    const float a = *(const float *)pa;
    const float b = *(const float *)pb;
    return (a > b) - (a < b);
}

int pan_tompkins_process(pt_context_t *ctx, const float *x, int count,
                         pt_beat_t *beats, int max_beats)
{
    int nbeats = 0;

    for (int i = 0; i < count; i++) {
        const float v = x[i];

        /* --- 60 Hz notch (direct-form II), ahead of the bandpass --- */
        const float nz = NOTCH_B0 * v + NOTCH_B1 * ctx->notch_x1 + NOTCH_B2 * ctx->notch_x2
                       - NOTCH_A1 * ctx->notch_y1 - NOTCH_A2 * ctx->notch_y2;
        ctx->notch_x2 = ctx->notch_x1;
        ctx->notch_x1 = v;
        ctx->notch_y2 = ctx->notch_y1;
        ctx->notch_y1 = nz;

        /* --- low-pass biquad (direct-form II) --- */
        const float lp = LP_B0 * nz + LP_B1 * ctx->lp_x1 + LP_B2 * ctx->lp_x2
                       - LP_A1 * ctx->lp_y1 - LP_A2 * ctx->lp_y2;
        ctx->lp_x2 = ctx->lp_x1;
        ctx->lp_x1 = nz;
        ctx->lp_y2 = ctx->lp_y1;
        ctx->lp_y1 = lp;

        /* --- high-pass biquad, 1st section (input = low-pass output) --- */
        const float hp = HP_B0 * lp + HP_B1 * ctx->hp_x1 + HP_B2 * ctx->hp_x2
                       - HP_A1 * ctx->hp_y1 - HP_A2 * ctx->hp_y2;
        ctx->hp_x2 = ctx->hp_x1;
        ctx->hp_x1 = lp;
        ctx->hp_y2 = ctx->hp_y1;
        ctx->hp_y1 = hp;

        /* --- high-pass biquad, 2nd section (cascade => 4th order) --- */
        const float hp2 = HP2_B0 * hp + HP2_B1 * ctx->hp2_x1 + HP2_B2 * ctx->hp2_x2
                        - HP2_A1 * ctx->hp2_y1 - HP2_A2 * ctx->hp2_y2;
        ctx->hp2_x2 = ctx->hp2_x1;
        ctx->hp2_x1 = hp;
        ctx->hp2_y2 = ctx->hp2_y1;
        ctx->hp2_y1 = hp2;

        /* --- derivative: (2x[n] + x[n-1] - x[n-3] - 2x[n-4]) / 8 --- */
        const float der = (2.0f * hp2 + ctx->d1 - ctx->d3 - 2.0f * ctx->d4) * 0.125f;
        ctx->d4 = ctx->d3;
        ctx->d3 = ctx->d2;
        ctx->d2 = ctx->d1;
        ctx->d1 = hp2;

        /* --- squaring --- */
        const float sq = der * der;

        /* --- moving-window integrator (150 ms) --- */
        ctx->mwi_sum += sq - ctx->mwi_buf[ctx->mwi_head];
        ctx->mwi_buf[ctx->mwi_head] = sq;
        ctx->mwi_head = (ctx->mwi_head + 1) % PT_MWI_WINDOW;
        const float mwi = ctx->mwi_sum / (float)PT_MWI_WINDOW;

        const uint32_t idx = ctx->index++;

        /* --- learning phase: collect MWI for robust threshold init --- */
        if (idx < PT_LEARN) {
            ctx->learn_buf[idx] = mwi;
            if (idx == PT_LEARN - 1) {
                /* Sort the learning-window integrator values in place, then
                 * seed SPKI from the 95th percentile and NPKI from the median.
                 * Using the max made a single motion spike in the first 2 s
                 * set the threshold too high and reject every real R wave. */
                qsort(ctx->learn_buf, PT_LEARN, sizeof(ctx->learn_buf[0]),
                      cmp_float_asc);
                ctx->spki = ctx->learn_buf[(int)(0.95f * PT_LEARN)];
                ctx->npki = ctx->learn_buf[(int)(0.50f * PT_LEARN)];
                ctx->thr1 = ctx->npki + 0.25f * (ctx->spki - ctx->npki);
                ctx->thr2 = 0.5f * ctx->thr1;
            }
            continue;
        }

        /* --- threshold recovery: decay SPKI toward the noise floor ---
         * A motion or electrode-pop artifact can cross thr1 and inflate SPKI
         * far above the real QRS amplitude. Once that happens no beat clears
         * thr1, so the detector stays quiet for the rest of the recording.
         * Decay SPKI toward NPKI whenever no QRS has been detected for >1.5 s
         * — including before the first beat, since the learning phase can be
         * poisoned by a start-up transient and no beat may ever be detected. */
        const uint32_t since_qrs = (ctx->last_qrs_index != 0)
                                 ? (idx - ctx->last_qrs_index) : idx;
        if (since_qrs > (uint32_t)(PT_THRESH_DECAY_START_S * PT_FS_HZ)) {
            ctx->spki = ctx->npki + PT_THRESH_DECAY * (ctx->spki - ctx->npki);
            ctx->thr1 = ctx->npki + 0.25f * (ctx->spki - ctx->npki);
            ctx->thr2 = 0.5f * ctx->thr1;
        }

        /* --- peak tracking (respect refractory period) --- */
        if (idx - ctx->last_peak_index >= PT_REFRACTORY) {
            if (mwi > ctx->peak_amp) {
                ctx->peak_amp = mwi;
                ctx->peak_index = idx;
            }
            /* A peak is complete once the integrator falls to half its max. */
            if (mwi < 0.5f * ctx->peak_amp && ctx->peak_amp > 0.0f) {
                const float    amp = ctx->peak_amp;
                const uint32_t pi  = ctx->peak_index;
                ctx->peak_amp = 0.0f;

                /* T-wave rejection: within 360 ms of last QRS and
                 * amplitude below half the previous QRS amplitude. */
                const bool twave = (ctx->last_qrs_index != 0 &&
                                    (pi - ctx->last_qrs_index) < (uint32_t)(0.36f * PT_FS_HZ) &&
                                    amp < 0.5f * ctx->last_qrs_amp);

                if (amp > ctx->thr1 && !twave) {
                    uint32_t rr = 0;
                    if (ctx->last_qrs_index != 0) {
                        rr = (uint32_t)((pi - ctx->last_qrs_index) * 1000.0f / PT_FS_HZ);
                    }
                    if (nbeats < max_beats) {
                        beats[nbeats].index = pi;
                        beats[nbeats].rr_ms = rr;
                        nbeats++;
                    }
                    ctx->last_qrs_index = pi;
                    ctx->last_qrs_amp = amp;
                    ctx->spki = 0.125f * amp + 0.875f * ctx->spki;
                } else {
                    ctx->npki = 0.125f * amp + 0.875f * ctx->npki;
                }

                /* Safety clamp: stop NPKI ratcheting up toward the signal
                 * level during stretches with no detected beats, which would
                 * push thr1 above real R waves and dead-lock the detector. */
                if (ctx->npki > 0.5f * ctx->spki) {
                    ctx->npki = 0.5f * ctx->spki;
                }

                ctx->thr1 = ctx->npki + 0.25f * (ctx->spki - ctx->npki);
                ctx->thr2 = 0.5f * ctx->thr1;
                ctx->last_peak_index = pi;
            }
        }
    }

    return nbeats;
}
