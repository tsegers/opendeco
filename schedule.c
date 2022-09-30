/* SPDX-License-Identifier: MIT-0 */

#include <assert.h>
#include <math.h>

#include "schedule.h"

#define SAFETY_STOP_DEPTH (abs_depth(msw_to_bar(6)))
#define SWITCH_INTERMEDIATE 1

const gas_t *best_gas(const double depth, const gas_t *gasses, const int nof_gasses)
{
    const gas_t *best = NULL;
    double mod_best = -1;

    for (int i = 0; i < nof_gasses; i++) {
        double mod = gas_mod(&gasses[i]);

        if (depth <= mod && (mod_best == -1 || mod < mod_best)) {
            best = &gasses[i];
            mod_best = mod;
        }
    }

    return best;
}

const gas_t *next_gas(const double depth, const gas_t *gasses, const int nof_gasses)
{
    const gas_t *next = NULL;
    double mod_best = 0;

    for (int i = 0; i < nof_gasses; i++) {
        double mod = gas_mod(&gasses[i]);

        if (depth > mod && mod > mod_best) {
            next = &gasses[i];
            mod_best = mod;
        }
    }

    return next;
}

int direct_ascent(const decostate_t *ds, const double depth, const double time, const gas_t *gas)
{
    decostate_t ds_ = *ds;
    assert(ds_.firststop == -1);

    add_segment_ascdec(&ds_, depth, SURFACE_PRESSURE, time, gas);

    return ceiling(&ds_, ds_.gfhi) <= SURFACE_PRESSURE;
}

void simulate_dive(decostate_t *ds, waypoint_t *waypoints, const int nof_waypoints, segment_callback_t seg_cb)
{
    double depth = SURFACE_PRESSURE;
    double runtime = 0;

    for (int i = 0; i < nof_waypoints; i++) {
        double d = waypoints[i].depth;
        double t = waypoints[i].time;
        const gas_t *g = waypoints[i].gas;

        if (d != depth) {
            runtime += add_segment_ascdec(ds, depth, d, t, g);
            depth = d;
        } else {
            runtime += add_segment_const(ds, d, t, g);
        }

        seg_cb(ds, (waypoint_t){.depth = d, .time = t, .gas = g}, SEG_DIVE);
    }
}

double calc_ndl(decostate_t *ds, const double depth, const double ascrate, const gas_t *gas)
{
    decostate_t ds_ = *ds;
    double ndl = 0;

    while (ndl < 360) {
        double tmp = add_segment_const(&ds_, depth, 1, gas);

        if (!direct_ascent(&ds_, depth, gauge_depth(depth) / ascrate, gas))
            break;

        ndl += tmp;
    }

    return ndl;
}

void extend_to_ndl(decostate_t *ds, const double depth, const double ascrate, const gas_t *gas,
                   segment_callback_t seg_cb)
{
    double ndl = calc_ndl(ds, depth, ascrate, gas);

    /* add segment to reach ndl */
    if (ndl) {
        add_segment_const(ds, depth, ndl, gas);
        seg_cb(ds, (waypoint_t){.depth = depth, .time = ndl, .gas = gas}, SEG_NDL);
    }

    /* either ascend directly or make a safety stop */
    if (depth < SAFETY_STOP_DEPTH || ds->max_depth < abs_depth(msw_to_bar(10))) {
        /* surface */
        add_segment_ascdec(ds, depth, SURFACE_PRESSURE, gauge_depth(depth) / ascrate, gas);
        seg_cb(ds, (waypoint_t){.depth = SURFACE_PRESSURE, .time = gauge_depth(depth) / ascrate, .gas = gas},
               SEG_SURFACE);
    } else {
        /* ascend to safety stop */
        add_segment_ascdec(ds, depth, SAFETY_STOP_DEPTH, (depth - SAFETY_STOP_DEPTH) / ascrate, gas);
        seg_cb(ds, (waypoint_t){.depth = SAFETY_STOP_DEPTH, .time = (depth - SAFETY_STOP_DEPTH) / ascrate, .gas = gas},
               SEG_TRAVEL);

        /* stop for 3 minutes */
        add_segment_const(ds, SAFETY_STOP_DEPTH, 3, gas);
        seg_cb(ds, (waypoint_t){.depth = SAFETY_STOP_DEPTH, .time = 3, .gas = gas}, SEG_SAFETY_STOP);

        /* surface */
        add_segment_ascdec(ds, SAFETY_STOP_DEPTH, SURFACE_PRESSURE, gauge_depth(SAFETY_STOP_DEPTH) / ascrate, gas);
        seg_cb(ds,
               (waypoint_t){.depth = SURFACE_PRESSURE, .time = gauge_depth(SAFETY_STOP_DEPTH) / ascrate, .gas = gas},
               SEG_SURFACE);
    }
}

decoinfo_t calc_deco(decostate_t *ds, const double start_depth, const gas_t *start_gas, const gas_t *deco_gasses,
                     const int nof_gasses, segment_callback_t seg_cb)
{
    decoinfo_t ret = {.tts = 0, .ndl = 0};

    /* setup start parameters */
    double depth = start_depth;
    const gas_t *gas = start_gas;

    const double asc_per_min = msw_to_bar(9);

    /* check if direct ascent is possible */
    if (direct_ascent(ds, depth, gauge_depth(depth) / asc_per_min, gas)) {
        ret.ndl = calc_ndl(ds, depth, asc_per_min, gas);
        return ret;
    }

    /* determine first stop */
    double current_gf = get_gf(ds, depth);

    if (ds->firststop == -1)
        ds->firststop = ceiling(ds, current_gf);

    /* switch to best deco gas if there is one available */
    const gas_t *best = best_gas(depth, deco_gasses, nof_gasses);

    if (best)
        gas = best;

    /* alternate between ascending and stopping */
    for (;;) {
        /* ascend */
        for (;;) {
            double stopdep = ceiling(ds, current_gf);
            const gas_t *next = next_gas(depth, deco_gasses, nof_gasses);

            if (SWITCH_INTERMEDIATE && next) {
                /* determine switch depth */
                double switch_depth = round_ceiling(ds, next->mod) - ds->ceil_multiple;
                assert(switch_depth <= next->mod);

                if (switch_depth > stopdep) {
                    /* ascend to switch depth */
                    ret.tts += add_segment_ascdec(ds, depth, switch_depth, (depth - switch_depth) / asc_per_min, gas);
                    seg_cb(
                        ds,
                        (waypoint_t){.depth = switch_depth, .time = (depth - switch_depth) / asc_per_min, .gas = gas},
                        SEG_TRAVEL);

                    depth = switch_depth;
                    current_gf = get_gf(ds, depth);

                    /* switch gas */
                    gas = next;

                    add_segment_const(ds, switch_depth, 1, gas);
                    seg_cb(ds, (waypoint_t){.depth = depth, .time = 1, .gas = gas}, SEG_GAS_SWITCH);

                    continue;
                }
            }

            ret.tts += add_segment_ascdec(ds, depth, stopdep, (depth - stopdep) / asc_per_min, gas);

            if (stopdep <= SURFACE_PRESSURE)
                seg_cb(ds, (waypoint_t){.depth = stopdep, .time = (depth - stopdep) / asc_per_min, .gas = gas},
                       SEG_SURFACE);
            else
                seg_cb(ds, (waypoint_t){.depth = stopdep, .time = (depth - stopdep) / asc_per_min, .gas = gas},
                       SEG_TRAVEL);

            depth = stopdep;
            current_gf = get_gf(ds, depth);

            /* if the ceiling moved while we ascended, keep ascending */
            if (depth > ceiling(ds, current_gf))
                continue;

            break;
        }

        /* terminate if we surfaced */
        if (depth <= SURFACE_PRESSURE)
            break;

        /* switch to better gas if available */
        const gas_t *best = best_gas(depth, deco_gasses, nof_gasses);

        if (best)
            gas = best;

        /* stop */
        double stoplen = 0;

        while (ceiling(ds, current_gf) == depth)
            stoplen += add_segment_const(ds, depth, 1, gas);

        ret.tts += stoplen;
        seg_cb(ds, (waypoint_t){.depth = depth, .time = stoplen, .gas = gas}, SEG_DECO_STOP);
    }

    return ret;
}
