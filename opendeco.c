/* SPDX-License-Identifier: MIT-0 */

#include <locale.h>
#include <math.h>
#include <wchar.h>

#include "deco.h"
#include "schedule.h"
#include "output.h"

#define MOD_OXY (abs_depth(msw_to_bar(6)))

void print_segment_callback(const decostate_t *ds, const waypoint_t wp, segtype_t type)
{
    static double last_depth;
    static double runtime;

    wchar_t sign;

    runtime += wp.time;

    if (wp.depth < last_depth)
        sign = ASC;
    else if (wp.depth > last_depth)
        sign = DEC;
    else
        sign = LVL;

    if (type != SEG_TRAVEL)
        print_planline(sign, wp.depth, wp.time, runtime, wp.gas);

    last_depth = wp.depth;
}

int main(int argc, const char *argv[])
{
    setlocale(LC_ALL, "en_US.utf8");

    /* setup */
    decostate_t ds;
    init_decostate(&ds, 30, 75, msw_to_bar(3));

    const gas_t ean32 = gas_new(32, 0, MOD_AUTO);

    /* simulate dive */
    waypoint_t waypoints[] = {
        {.depth = abs_depth(msw_to_bar(30)), .time = 3.333,   &ean32},
        {.depth = abs_depth(msw_to_bar(30)), .time = 116.666, &ean32},
    };

    print_planhead();
    simulate_dive(&ds, waypoints, len(waypoints), &print_segment_callback);

    /* generate deco schedule */
    double depth = waypoints[len(waypoints) - 1].depth;
    const gas_t *gas = waypoints[len(waypoints) - 1].gas;

    gas_t deco_gasses[] = {
        /* gas_new(40, 0, MOD_AUTO), */
        gas_new(50, 0, MOD_AUTO),
        /* gas_new(100, 0, MOD_OXY), */
    };

    /* determine @+5 TTS */
    decostate_t ds_ = ds;
    add_segment_const(&ds_, depth, 5, gas);
    decoinfo_t di_plus5 = calc_deco(&ds_, depth, gas, deco_gasses, len(deco_gasses), NULL);

    /* print actual deco schedule */
    decoinfo_t di = calc_deco(&ds, depth, gas, deco_gasses, len(deco_gasses), &print_segment_callback);

    /* output deco info and disclaimer */
    wprintf(L"\nNDL: %i TTS: %i TTS @+5: %i\n", (int) floor(di.ndl), (int) ceil(di.tts), (int) ceil(di_plus5.tts));
    print_planfoot(&ds);

    return 0;
}
