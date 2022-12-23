/* SPDX-License-Identifier: MIT-0 */

#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "deco.h"
#include "opendeco-cli.h"
#include "opendeco-conf.h"
#include "output.h"
#include "schedule.h"

#define MOD_OXY (abs_depth(msw_to_bar(6)))

#define RMV_DIVE_DEFAULT 20
#define RMV_DECO_DEFAULT 15
#define SHOW_TRAVEL_DEFAULT 0

double RMV_DIVE = RMV_DIVE_DEFAULT;
double RMV_DECO = RMV_DECO_DEFAULT;

int SHOW_TRAVEL = SHOW_TRAVEL_DEFAULT;

static struct gas_usage {
    const gas_t *gas;
    double usage;
} gas_usage[10];

int register_gas_use(double depth, double time, const gas_t *gas, double rmv)
{
    double usage = depth * time * rmv;

    for (int i = 0; i < 10; i++) {
        if (!gas_usage[i].gas) {
            gas_usage[i] = (struct gas_usage){
                .gas = gas,
                .usage = usage,
            };
            return 0;
        } else if (gas_usage[i].gas == gas) {
            gas_usage[i].usage += usage;
            return 0;
        }
    }

    return -1;
}

void print_gas_use(void)
{
    static char gasbuf[12];

    wprintf(L"\n");

    for (int i = 0; i < 10; i++) {
        if (gas_usage[i].gas) {
            format_gas(gasbuf, len(gasbuf), gas_usage[i].gas);
            strcat(gasbuf, ":");
            wprintf(L"%-12s%5i%lc\n", gasbuf, (int) ceil(gas_usage[i].usage), LTR);
        }
    }
}

void print_segment_callback_fn(const decostate_t *ds, waypoint_t wp, segtype_t type, void *arg)
{
    static double last_depth;
    static double runtime;

    wchar_t sign;

    /* first time initialization */
    if (!last_depth)
        last_depth = SURFACE_PRESSURE;

    runtime += wp.time;

    if (wp.depth < last_depth)
        sign = ASC;
    else if (wp.depth > last_depth)
        sign = DEC;
    else
        sign = LVL;

    if (SHOW_TRAVEL || type != SEG_TRAVEL)
        print_planline(sign, wp.depth, wp.time, runtime, wp.gas);

    /* register gas use */
    double avg_seg_depth = wp.depth == last_depth ? last_depth : (wp.depth + last_depth) / 2;
    double rmv = type == SEG_DIVE ? RMV_DIVE : RMV_DECO;

    register_gas_use(avg_seg_depth, wp.time, wp.gas, rmv);

    last_depth = wp.depth;
}

int parse_gasses(gas_t **gasses, char *str)
{
    if (!str) {
        *gasses = NULL;
        return 0;
    }

    /* count number of gasses in string */
    int nof_gasses = 1;

    for (int c = 0; str[c]; c++)
        if (str[c] == ',')
            nof_gasses++;

    /* allocate gas array */
    gas_t *deco_gasses = malloc(nof_gasses * sizeof(gas_t));

    /* fill gas array */
    char *gas_str = NULL;
    int gas_idx = 0;

    for (;;) {
        if (!gas_str)
            gas_str = strtok(str, ",");
        else
            gas_str = strtok(NULL, ",");

        if (!gas_str)
            break;

        scan_gas(&deco_gasses[gas_idx], gas_str);
        gas_idx++;
    }

    *gasses = deco_gasses;
    return nof_gasses;
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "en_US.utf8");

    /* argp */
    char *gas_default = strdup("Air");
    char *decogasses_default = strdup("");

    struct arguments arguments = {
        .depth = -1,
        .time = -1,
        .gas = gas_default,
        .gflow = 30,
        .gfhigh = 75,
        .decogasses = decogasses_default,
        .SURFACE_PRESSURE = SURFACE_PRESSURE_DEFAULT,
        .SWITCH_INTERMEDIATE = SWITCH_INTERMEDIATE_DEFAULT,
        .LAST_STOP_AT_SIX = LAST_STOP_AT_SIX_DEFAULT,
        .RMV_DIVE = RMV_DIVE_DEFAULT,
        .RMV_DECO = RMV_DECO_DEFAULT,
        .SHOW_TRAVEL = SHOW_TRAVEL_DEFAULT,
    };

    opendeco_conf_parse("opendeco.toml", &arguments);
    opendeco_argp_parse(argc, argv, &arguments);

    /* apply global options */
    SURFACE_PRESSURE = arguments.SURFACE_PRESSURE;
    SWITCH_INTERMEDIATE = arguments.SWITCH_INTERMEDIATE;
    LAST_STOP_AT_SIX = arguments.LAST_STOP_AT_SIX;
    RMV_DIVE = arguments.RMV_DIVE;
    RMV_DECO = arguments.RMV_DECO;
    SHOW_TRAVEL = arguments.SHOW_TRAVEL;

    /* setup */
    decostate_t ds;
    init_decostate(&ds, arguments.gflow, arguments.gfhigh, msw_to_bar(3));
    double dec_per_min = msw_to_bar(9);

    gas_t bottom_gas;
    scan_gas(&bottom_gas, arguments.gas);

    gas_t *deco_gasses;
    int nof_gasses = parse_gasses(&deco_gasses, arguments.decogasses);

    /* override oxygen mod */
    for (int i = 0; i < nof_gasses; i++)
        if (gas_o2(&deco_gasses[i]) == 100)
            deco_gasses[i].mod = MOD_OXY;

    /* simulate dive */
    double descent_time = msw_to_bar(arguments.depth) / dec_per_min;
    double bottom_time = max(1, arguments.time - descent_time);

    waypoint_t waypoints[] = {
        {.depth = abs_depth(msw_to_bar(arguments.depth)), .time = descent_time, &bottom_gas},
        {.depth = abs_depth(msw_to_bar(arguments.depth)), .time = bottom_time,  &bottom_gas},
    };

    waypoint_callback_t print_segment_callback = {
        .fn = &print_segment_callback_fn,
        .arg = NULL,
    };

    print_planhead();
    simulate_dive(&ds, waypoints, len(waypoints), &print_segment_callback);

    /* generate deco schedule */
    double depth = waypoints[len(waypoints) - 1].depth;
    const gas_t *gas = waypoints[len(waypoints) - 1].gas;

    /* determine @+5 TTS */
    decostate_t ds_ = ds;
    add_segment_const(&ds_, depth, 5, gas);
    decoinfo_t di_plus5 = calc_deco(&ds_, depth, gas, deco_gasses, nof_gasses, NULL);

    /* print actual deco schedule */
    decoinfo_t di = calc_deco(&ds, depth, gas, deco_gasses, nof_gasses, &print_segment_callback);

    /* output deco info and disclaimer */
    print_gas_use();
    wprintf(L"\nNDL: %i TTS: %i TTS @+5: %i\n", (int) floor(di.ndl), (int) ceil(di.tts), (int) ceil(di_plus5.tts));
    print_planfoot(&ds);

    /* cleanup */
    free(deco_gasses);
    free(arguments.gas);
    free(arguments.decogasses);

    return 0;
}
