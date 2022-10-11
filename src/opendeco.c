/* SPDX-License-Identifier: MIT-0 */

#include <argp.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "deco.h"
#include "schedule.h"
#include "output.h"

#define MOD_OXY (abs_depth(msw_to_bar(6)))

#ifndef VERSION
#define VERSION "unknown version"
#endif

/* argp settings */
static char args_doc[] = "";
static char doc[] = "Implementation of Buhlmann ZH-L16 with Gradient Factors:"
                    "\vExamples:\n\n"
                    "\t./opendeco -d 18 -t 60 -g Air\n"
                    "\t./opendeco -d 30 -t 60 -g EAN32\n"
                    "\t./opendeco -d 40 -t 120 -g 21/35 -l 20 -h 80 --decogasses Oxygen,EAN50\n";
const char *argp_program_bug_address = "<~tsegers/opendeco@lists.sr.ht> or https://todo.sr.ht/~tsegers/opendeco";
const char *argp_program_version = "opendeco " VERSION;

static struct argp_option options[] = {
    {"depth",      'd', "NUMBER", 0,                   "Set the depth of the dive in meters",                          0},
    {"time",       't', "NUMBER", 0,                   "Set the time of the dive in minutes",                          1},
    {"gas",        'g', "STRING", 0,                   "Set the bottom gas used during the dive, defaults to Air",     2},
    {"pressure",   'p', "STRING", 0,                   "Set the surface air pressure, defaults to 1.01325bar or 1atm", 3},
    {0,            's', 0,        OPTION_ARG_OPTIONAL, "Only switch gas at deco stops",                                4},
    {"gflow",      'l', "NUMBER", 0,                   "Set the gradient factor at the first stop, defaults to 30",    5},
    {"gfhigh",     'h', "NUMBER", 0,                   "Set the gradient factor at the surface, defaults to 75",       6},
    {"decogasses", 'G', "LIST",   0,                   "Set the gasses available for deco",                            7},
    {0,            0,   0,        0,                   0,                                                              0}
};

struct arguments {
    double depth;
    double time;
    char *gas;
    int gflow;
    int gfhigh;
    char *decogasses;
    double SURFACE_PRESSURE;
    int SWITCH_INTERMEDIATE;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key) {
    case 'd':
        arguments->depth = arg ? atof(arg) : -1;
        break;
    case 't':
        arguments->time = arg ? atof(arg) : -1;
        break;
    case 'g':
        arguments->gas = arg;
        break;
    case 'p':
        arguments->SURFACE_PRESSURE = arg ? atof(arg) : -1;
        break;
    case 's':
        arguments->SWITCH_INTERMEDIATE = 0;
        break;
    case 'G':
        arguments->decogasses = arg;
        break;
    case 'l':
        arguments->gflow = arg ? atoi(arg) : 100;
        break;
    case 'h':
        arguments->gfhigh = arg ? atoi(arg) : 100;
        break;
    case ARGP_KEY_END:
        if (arguments->depth < 0 || arguments->time < 0) {
            argp_state_help(state, stderr, ARGP_HELP_USAGE);
            argp_failure(state, 1, 0, "Options -d and -t are required. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        if (arguments->SURFACE_PRESSURE < 0) {
            argp_state_help(state, stderr, ARGP_HELP_USAGE);
            argp_failure(state, 1, 0, "Surface air pressure must be positive. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

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

    while (1) {
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
    struct arguments arguments = {
        .depth = -1,
        .time = -1,
        .gas = "Air",
        .gflow = 30,
        .gfhigh = 75,
        .decogasses = "",
        .SURFACE_PRESSURE = 0,
        .SWITCH_INTERMEDIATE = 1,
    };

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    /* apply global options */
    if (arguments.SURFACE_PRESSURE > 0)
        SURFACE_PRESSURE = arguments.SURFACE_PRESSURE;

    SWITCH_INTERMEDIATE = arguments.SWITCH_INTERMEDIATE;

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
    wprintf(L"\nNDL: %i TTS: %i TTS @+5: %i\n", (int) floor(di.ndl), (int) ceil(di.tts), (int) ceil(di_plus5.tts));
    print_planfoot(&ds);

    return 0;
}