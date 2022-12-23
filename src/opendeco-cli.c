/* SPDX-License-Identifier: MIT-0 */

#include <argp.h>
#include <stdlib.h>

#include "opendeco-cli.h"

static char args_doc[] = "";
static char doc[] = "Implementation of Buhlmann ZH-L16 with Gradient Factors:"
                    "\vExamples:\n\n"
                    "  ./opendeco -d 18 -t 60 -g Air\n"
                    "  ./opendeco -d 30 -t 60 -g EAN32\n"
                    "  ./opendeco -d 40 -t 120 -g 21/35 -L 20 -H 80 --decogasses Oxygen,EAN50\n";
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
    {"gflow",      'L', "NUMBER", 0,                   "Set the gradient factor at the first stop, defaults to 30",       5 },
    {"gfhigh",     'H', "NUMBER", 0,                   "Set the gradient factor at the surface, defaults to 75",          6 },
    {"decogasses", 'G', "LIST",   0,                   "Set the gasses available for deco",                               7 },
    {0,            'S', 0,        OPTION_ARG_OPTIONAL, "Only switch gas at deco stops",                                   8 },
    {0,            '6', 0,        OPTION_ARG_OPTIONAL, "Perform last deco stop at 6m",                                    9 },
    {"decormv",    'R', "NUMBER", 0,                   "Set the RMV during the deco portion of the dive, defaults to 15", 10},
    {"showtravel", 'T', 0,        0,                   "Show travel segments in deco plan",                               11},

    {0,            0,   0,        0,                   "Informational options:",                                          0 },
    {0,            0,   0,        0,                   0,                                                                 0 }
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
    case 'r':
        arguments->RMV_DIVE = arg ? atof(arg) : -1;
        break;
    case 'L':
        arguments->gflow = arg ? atoi(arg) : 100;
        break;
    case 'H':
        arguments->gfhigh = arg ? atoi(arg) : 100;
        break;
    case 'G':
        arguments->decogasses = arg;
        break;
    case 'S':
        arguments->SWITCH_INTERMEDIATE = 0;
        break;
    case '6':
        arguments->LAST_STOP_AT_SIX = 1;
        break;
    case 'R':
        arguments->RMV_DECO = arg ? atof(arg) : -1;
        break;
    case 'T':
        arguments->SHOW_TRAVEL = 1;
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

void opendeco_argp_parse(int argc, char *argv[], struct arguments *arguments)
{
    static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};
    argp_parse(&argp, argc, argv, 0, 0, arguments);
}
