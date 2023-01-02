/* SPDX-License-Identifier: MIT-0 */

#ifndef OPENDECOCONF_H
#define OPENDECOCONF_H

#ifndef VERSION
#define VERSION "unknown version"
#endif

/* types */
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
    int SHOW_TRAVEL;
};

int opendeco_conf_parse(const char *confpath, struct arguments *arguments);

#endif /* end of include guard: OPENDECOCONF_H */
