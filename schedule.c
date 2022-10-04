/* SPDX-License-Identifier: MIT-0 */

#include <assert.h>
#include <math.h>

#include "schedule.h"

#define SWITCH_INTERMEDIATE 1

#define STOPLEN_ROUGH 10
#define STOPLEN_FINE 1

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

    add_segment_ascdec(&ds_, depth, abs_depth(0), time, gas);

    return gauge_depth(ceiling(&ds_, ds_.gfhi)) <= 0;
}

void simulate_dive(decostate_t *ds, waypoint_t *waypoints, const int nof_waypoints, waypoint_callback_t wp_cb)
{
    double depth = abs_depth(0);
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

        if (wp_cb)
            wp_cb(ds, (waypoint_t){.depth = d, .time = t, .gas = g}, SEG_DIVE);
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

double calc_stoplen_rough(const decostate_t *ds, const double depth, const double current_gf, const gas_t *gas)
{
    decostate_t ds_ = *ds;
    double stoplen = 0;

    for (;;) {
        double tmp = add_segment_const(&ds_, depth, STOPLEN_ROUGH, gas);

        if (ceiling(&ds_, current_gf) != depth)
            break;

        stoplen += tmp;
    }

    return stoplen;
}

double deco_stop(decostate_t *ds, const double depth, const double current_gf, const gas_t *gas)
{
    double stoplen = 0;

    /* rough steps */
    double stoplen_rough = calc_stoplen_rough(ds, depth, current_gf, gas);

    if (stoplen_rough) {
        add_segment_const(ds, depth, stoplen_rough, gas);
        stoplen += stoplen_rough;
    }

    /* fine steps */
    while (ceiling(ds, current_gf) == depth)
        stoplen += add_segment_const(ds, depth, STOPLEN_FINE, gas);

    return stoplen;
}

int should_switch(const gas_t **next, double *switch_depth, const decostate_t *ds, const double depth,
                  const double next_stop, const gas_t *deco_gasses, const int nof_gasses)
{
    /* check if we switch at MOD or just at stops */
    if (!SWITCH_INTERMEDIATE)
        return 0;

    /* check if there is a gas to switch to */
    *next = next_gas(depth, deco_gasses, nof_gasses);

    if (*next == NULL)
        return 0;

    /* check if the switch happens before the current ceiling */
    *switch_depth = round_ceiling(ds, (*next)->mod) - ds->ceil_multiple;
    assert(*switch_depth <= (*next)->mod);

    if (*switch_depth <= next_stop)
        return 0;

    return 1;
}

decoinfo_t calc_deco(decostate_t *ds, const double start_depth, const gas_t *start_gas, const gas_t *deco_gasses,
                     const int nof_gasses, waypoint_callback_t wp_cb)
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
        /* extra bookkeeping because waypoints and segments do not match 1:1 */
        double last_waypoint_depth = depth;
        double waypoint_time;

        /* ascend */
        for (;;) {
            /* determine next stop */
            double next_stop = ceiling(ds, current_gf);

            /* find out if we need to switch gas on the way */
            const gas_t *next;
            double switch_depth;

            if (should_switch(&next, &switch_depth, ds, depth, next_stop, deco_gasses, nof_gasses)) {
                /* ascend to gas switch */
                ret.tts += add_segment_ascdec(ds, depth, switch_depth, fabs(depth - switch_depth) / asc_per_min, gas);
                depth = switch_depth;
                current_gf = get_gf(ds, depth);

                /*
                 * since we're stopping for a switch next, this is the last of
                 * any number of consecutive travel segments and the waypoint
                 * callback should be called.
                 */
                waypoint_time = fabs(last_waypoint_depth - depth) / asc_per_min;

                if (wp_cb)
                    wp_cb(ds, (waypoint_t){.depth = depth, .time = waypoint_time, .gas = gas}, SEG_TRAVEL);

                last_waypoint_depth = depth;

                /* switch gas */
                gas = next;

                ret.tts += add_segment_const(ds, switch_depth, 1, gas);

                if (wp_cb)
                    wp_cb(ds, (waypoint_t){.depth = depth, .time = 1, .gas = gas}, SEG_GAS_SWITCH);

                continue;
            }

            /* ascend to current ceiling */
            ret.tts += add_segment_ascdec(ds, depth, next_stop, fabs(depth - next_stop) / asc_per_min, gas);
            depth = next_stop;
            current_gf = get_gf(ds, depth);

            /* if the ceiling moved while we ascended, keep ascending */
            if (depth > ceiling(ds, current_gf))
                continue;

            /*
             * since we've actually reached the ceiling, this is the last of
             * any number of consecutive travel segments and the waypoint
             * callback should be called.
             */
            waypoint_time = fabs(last_waypoint_depth - depth) / asc_per_min;
            enum segtype_t segtype = depth <= abs_depth(0) ? SEG_SURFACE : SEG_TRAVEL;

            if (wp_cb)
                wp_cb(ds, (waypoint_t){.depth = depth, .time = waypoint_time, .gas = gas}, segtype);

            break;
        }

        /* terminate if we surfaced */
        if (depth <= abs_depth(0))
            break;

        /* switch to better gas if available */
        const gas_t *best = best_gas(depth, deco_gasses, nof_gasses);

        if (best)
            gas = best;

        /* stop */
        double stoplen = deco_stop(ds, depth, current_gf, gas);
        ret.tts += stoplen;

        if (wp_cb)
            wp_cb(ds, (waypoint_t){.depth = depth, .time = stoplen, .gas = gas}, SEG_DECO_STOP);
    }

    return ret;
}
