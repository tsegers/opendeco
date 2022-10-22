/* SPDX-License-Identifier: MIT-0 */

#include <assert.h>
#include <math.h>

#include "schedule.h"

#define STOPLEN_ROUGH 10
#define STOPLEN_FINE 1

int SWITCH_INTERMEDIATE = SWITCH_INTERMEDIATE_DEFAULT;

const gas_t *best_gas(double depth, const gas_t *gasses, int nof_gasses)
{
    const gas_t *best = NULL;
    double mod_best = -1;

    for (int i = 0; i < nof_gasses; i++) {
        double mod = gas_mod(&gasses[i]);

        if (depth - mod < 1E-2 && (mod_best == -1 || mod < mod_best)) {
            best = &gasses[i];
            mod_best = mod;
        }
    }

    return best;
}

int direct_ascent(const decostate_t *ds, double depth, double time, const gas_t *gas)
{
    decostate_t ds_ = *ds;
    assert(ds_.firststop == -1);

    add_segment_ascdec(&ds_, depth, abs_depth(0), time, gas);

    return gauge_depth(ceiling(&ds_, ds_.gfhi)) <= 0;
}

void simulate_dive(decostate_t *ds, const waypoint_t *waypoints, int nof_waypoints, const waypoint_callback_t *wp_cb)
{
    double depth = abs_depth(0);

    for (int i = 0; i < nof_waypoints; i++) {
        double d = waypoints[i].depth;
        double t = waypoints[i].time;
        const gas_t *g = waypoints[i].gas;

        if (d != depth)
            add_segment_ascdec(ds, depth, d, t, g);
        else
            add_segment_const(ds, d, t, g);

        depth = d;

        if (wp_cb && wp_cb->fn)
            wp_cb->fn(ds, (waypoint_t){.depth = d, .time = t, .gas = g}, SEG_DIVE, wp_cb->arg);
    }
}

double calc_ndl(decostate_t *ds, double depth, double ascrate, const gas_t *gas)
{
    double ndl = 0;

    /* rough steps */
    decostate_t ds_ = *ds;

    while (ndl < 360) {
        double tmp = add_segment_const(&ds_, depth, STOPLEN_ROUGH, gas);

        if (!direct_ascent(&ds_, depth, gauge_depth(depth) / ascrate, gas))
            break;

        ndl += tmp;
    }

    /* fine steps */
    ds_ = *ds;

    if (ndl)
        add_segment_const(&ds_, depth, ndl, gas);

    while (ndl < 360) {
        double tmp = add_segment_const(&ds_, depth, STOPLEN_FINE, gas);

        if (!direct_ascent(&ds_, depth, gauge_depth(depth) / ascrate, gas))
            break;

        ndl += tmp;
    }

    return ndl;
}

double deco_stop(decostate_t *ds, double depth, double next_stop, double current_gf, const gas_t *gas)
{
    double stoplen = 0;

    /* rough steps */
    decostate_t ds_ = *ds;

    for (;;) {
        double tmp = add_segment_const(&ds_, depth, STOPLEN_ROUGH, gas);

        if (ceiling(&ds_, current_gf) <= next_stop)
            break;

        stoplen += tmp;
    }

    if (stoplen)
        add_segment_const(ds, depth, stoplen, gas);

    /* fine steps */
    while (ceiling(ds, current_gf) > next_stop)
        stoplen += add_segment_const(ds, depth, STOPLEN_FINE, gas);

    return stoplen;
}

static int surfaced(double depth)
{
    return fabs(depth - SURFACE_PRESSURE) < 1E-2;
}

decoinfo_t calc_deco(decostate_t *ds, double start_depth, const gas_t *start_gas, const gas_t *deco_gasses,
                     int nof_gasses, const waypoint_callback_t *wp_cb)
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

    double next_stop = abs_depth(ds->ceil_multiple * (ceil(gauge_depth(depth) / ds->ceil_multiple) - 1));
    double current_gf = get_gf(ds, next_stop);

    for (;;) {
        /* extra bookkeeping because waypoints and segments do not match 1:1 */
        double last_waypoint_depth = depth;
        double waypoint_time;

        while (ceiling(ds, current_gf) < next_stop && !surfaced(depth)) {
            /* switch to better gas if available */
            const gas_t *best = best_gas(depth, deco_gasses, nof_gasses);

            if (SWITCH_INTERMEDIATE && best && best != gas) {
                /* emit waypoint */
                waypoint_time = fabs(last_waypoint_depth - depth) / asc_per_min;

                if (wp_cb && wp_cb->fn)
                    wp_cb->fn(ds, (waypoint_t){.depth = depth, .time = waypoint_time, .gas = gas}, SEG_TRAVEL,
                              wp_cb->arg);

                last_waypoint_depth = depth;

                /* switch gas */
                gas = best;

                ret.tts += add_segment_const(ds, depth, 1, gas);

                if (wp_cb && wp_cb->fn)
                    wp_cb->fn(ds, (waypoint_t){.depth = depth, .time = 1, .gas = gas}, SEG_GAS_SWITCH, wp_cb->arg);

                continue;
            }

            /* ascend to next stop */
            ret.tts += add_segment_ascdec(ds, depth, next_stop, fabs(depth - next_stop) / asc_per_min, gas);
            depth = next_stop;

            /* make next stop shallower */
            next_stop -= ds->ceil_multiple;

            if (LAST_STOP_AT_SIX && next_stop < abs_depth(msw_to_bar(6)))
                next_stop = SURFACE_PRESSURE;

            /* recalculate gf */
            current_gf = get_gf(ds, next_stop);
        }

        if (ds->firststop == -1) {
            ds->firststop = depth;

            /*
             * if the first stop wasn't know yet during the previous call to
             * get_gf, the result was inaccurate and needs to be recalculated
             */
            current_gf = get_gf(ds, next_stop);

            /* if the new gf also allows us to ascend further, continue ascending */
            if (ceiling(ds, current_gf) < next_stop)
                continue;
        }

        /* emit waypoint */
        waypoint_time = fabs(last_waypoint_depth - depth) / asc_per_min;
        enum segtype_t segtype = surfaced(depth) ? SEG_SURFACE : SEG_TRAVEL;

        if (wp_cb && wp_cb->fn && waypoint_time)
            wp_cb->fn(ds, (waypoint_t){.depth = depth, .time = waypoint_time, .gas = gas}, segtype, wp_cb->arg);

        /* terminate if we surfaced */
        if (surfaced(depth))
            return ret;

        /* switch to better gas if available */
        const gas_t *best = best_gas(depth, deco_gasses, nof_gasses);

        if (best)
            gas = best;

        /* stop until ceiling rises above next stop */
        double stoplen = deco_stop(ds, depth, next_stop, current_gf, gas);

        ret.tts += stoplen;

        if (wp_cb && wp_cb->fn)
            wp_cb->fn(ds, (waypoint_t){.depth = depth, .time = stoplen, .gas = gas}, SEG_DECO_STOP, wp_cb->arg);
    }
}
