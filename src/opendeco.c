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

#define RMV_DIVE_DEFAULT 20
#define RMV_DECO_DEFAULT 15

#ifndef VERSION
#define VERSION "unknown version"
#endif

double RMV_DIVE = RMV_DIVE_DEFAULT;
double RMV_DECO = RMV_DECO_DEFAULT;

/* argp settings */
static char args_doc[] = "";
static char doc[] = "Implementation of Buhlmann ZH-L16 with Gradient Factors:"
                    "\vExamples:\n\n"
                    "  ./opendeco -d 18 -t 60 -g Air\n"
                    "  ./opendeco -d 30 -t 60 -g EAN32\n"
                    "  ./opendeco -d 40 -t 120 -g 21/35 -l 20 -h 80 --decogasses Oxygen,EAN50\n";
const char *argp_program_bug_address = "<~tsegers/opendeco@lists.sr.ht> or https://todo.sr.ht/~tsegers/opendeco";
const char *argp_program_version = "opendeco " VERSION;

static struct argp_option options[] = {
    {0,            0,   0,        0,                   "Dive options:",                                                   0 },
    {"depth",      'd', "NUMBER", 0,                   "Set the depth of the dive in meters",                             0 },
    {"time",       't', "NUMBER", 0,                   "Set the time of the dive in minutes",                             1 },
    {"gas",        'g', "STRING", 0,                   "Set the bottom gas used during the dive, defaults to Air",        2 },
    {"pressure",   'p', "NUMBER", 0,                   "Set the surface air pressure, defaults to 1.01325bar or 1atm",    3 },
    {"rmv",        'r', "NUMBER", 0,                   "Set the RMV during the dive portion of the dive, defaults to 20", 4 },

    {0,            0,   0,        0,                   "Deco options:",                                                   0 },
    {"gflow",      'l', "NUMBER", 0,                   "Set the gradient factor at the first stop, defaults to 30",       5 },
    {"gfhigh",     'h', "NUMBER", 0,                   "Set the gradient factor at the surface, defaults to 75",          6 },
    {"decogasses", 'G', "LIST",   0,                   "Set the gasses available for deco",                               7 },
    {0,            's', 0,        OPTION_ARG_OPTIONAL, "Only switch gas at deco stops",                                   8 },
    {0,            '6', 0,        OPTION_ARG_OPTIONAL, "Perform last deco stop at 6m",                                    9 },
    {"decormv",    'R', "NUMBER", 0,                   "Set the RMV during the deco portion of the dive, defaults to 15", 10},

    {0,            0,   0,        0,                   "Informational options:",                                          0 },
    {0,            0,   0,        0,                   0,                                                                 0 }
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
    int LAST_STOP_AT_SIX;
    double RMV_DIVE;
    double RMV_DECO;
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
    case '6':
        arguments->LAST_STOP_AT_SIX = 1;
        break;
    case 'r':
        arguments->RMV_DIVE = arg ? atof(arg) : -1;
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
    case 'R':
        arguments->RMV_DECO = arg ? atof(arg) : -1;
        break;
    case ARGP_KEY_END:
        if (arguments->depth < 0 || arguments->time < 0) {
            argp_state_help(state, stderr, ARGP_HELP_USAGE);
            argp_failure(state, 1, 0, "Options -d and -t are required. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        if (arguments->SURFACE_PRESSURE <= 0) {
            argp_failure(state, 1, 0, "Surface air pressure must be positive");
            exit(ARGP_ERR_UNKNOWN);
        }
        if (arguments->gflow > arguments->gfhigh) {
            argp_failure(state, 1, 0, "GF Low must not be greater than GF High");
            exit(ARGP_ERR_UNKNOWN);
        }
        if (arguments->RMV_DIVE <= 0) {
            argp_failure(state, 1, 0, "Dive RMV must be greater than 0");
            exit(ARGP_ERR_UNKNOWN);
        }
        if (arguments->RMV_DECO <= 0) {
            argp_failure(state, 1, 0, "Deco RMV must be greater than 0");
            exit(ARGP_ERR_UNKNOWN);
        }
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct gas_usage {
    const gas_t *gas;
    double usage;
} gas_usage[10];

int register_gas_use(const double depth, const double time, const gas_t *gas, const double rmv)
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

void print_gas_use()
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

void print_segment_callback_fn(const decostate_t *ds, const waypoint_t wp, segtype_t type, void *arg)
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

    if (type != SEG_TRAVEL)
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
    static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

    struct arguments arguments = {
        .depth = -1,
        .time = -1,
        .gas = "Air",
        .gflow = 30,
        .gfhigh = 75,
        .decogasses = "",
        .SURFACE_PRESSURE = SURFACE_PRESSURE_DEFAULT,
        .SWITCH_INTERMEDIATE = SWITCH_INTERMEDIATE_DEFAULT,
        .LAST_STOP_AT_SIX = LAST_STOP_AT_SIX_DEFAULT,
        .RMV_DIVE = RMV_DIVE_DEFAULT,
        .RMV_DECO = RMV_DECO_DEFAULT,
    };

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    /* apply global options */
    SURFACE_PRESSURE = arguments.SURFACE_PRESSURE;
    SWITCH_INTERMEDIATE = arguments.SWITCH_INTERMEDIATE;
    LAST_STOP_AT_SIX = arguments.LAST_STOP_AT_SIX;
    RMV_DIVE = arguments.RMV_DIVE;
    RMV_DECO = arguments.RMV_DECO;

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

    free(deco_gasses);
    return 0;
}
