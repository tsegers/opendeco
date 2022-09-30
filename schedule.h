/* SPDX-License-Identifier: MIT-0 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "deco.h"

typedef struct waypoint_t {
    double depth;
    double time;
    const gas_t *gas;
} waypoint_t;

typedef struct decoinfo_t {
    double ndl;
    double tts;
} decoinfo_t;

typedef enum segtype_t {
    SEG_DECO_STOP,
    SEG_DIVE,
    SEG_GAS_SWITCH,
    SEG_NDL,
    SEG_SAFETY_STOP,
    SEG_SURFACE,
    SEG_TRAVEL,
} segtype_t;

typedef void (*segment_callback_t)(const decostate_t *ds, const waypoint_t, const segtype_t);

const gas_t *best_gas(const double depth, const gas_t *gasses, const int nof_gasses);
const gas_t *next_gas(const double depth, const gas_t *gasses, const int nof_gasses);

int direct_ascent(const decostate_t *ds, const double depth, const double time, const gas_t *gas);
double calc_ndl(decostate_t *ds, const double depth, const double ascrate, const gas_t *gas);

void simulate_dive(decostate_t *ds, waypoint_t *waypoints, const int nof_waypoints, segment_callback_t seg_cb);

decoinfo_t calc_deco(decostate_t *ds, const double start_depth, const gas_t *start_gas, const gas_t *deco_gasses,
                     const int nof_gasses, segment_callback_t seg_cb);

#endif /* end of include guard: SCHEDULE_H */
