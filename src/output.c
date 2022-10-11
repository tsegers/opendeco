/* SPDX-License-Identifier: MIT-0 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "output.h"

void format_mm_ss(char *buf, const size_t buflen, const double time)
{
    double mm;
    double ss = round(modf(time, &mm) * 60);

    /* prevents 0.99999 minutes showing as 00:60 */
    mm += ss / 60;
    ss = (int) ss % 60;

    snprintf(buf, buflen, "%3i:%02i", (int) mm, (int) ss);
}

void format_gas(char *buf, const size_t buflen, const gas_t *gas)
{
    if (gas_o2(gas) == 21 && gas_he(gas) == 0)
        snprintf(buf, buflen, "Air");
    else if (gas_o2(gas) == 100)
        snprintf(buf, buflen, "Oxygen");
    else if (gas_he(gas) == 0)
        snprintf(buf, buflen, "Nitrox %i", gas_o2(gas));
    else
        snprintf(buf, buflen, "%i/%i", gas_o2(gas), gas_he(gas));
}

void scan_gas(gas_t *gas, char *str)
{
    int o2 = 0;
    int he = 0;

    if (!strcmp(str, "Air")) {
        *gas = gas_new(21, 0, MOD_AUTO);
        return;
    } else if (!strcmp(str, "Oxygen")) {
        *gas = gas_new(100, 0, MOD_AUTO);
        return;
    } else if (!strncmp(str, "EAN", strlen("EAN"))) {
        sscanf(str, "EAN%i", &o2);
    } else if (!strncmp(str, "Nitrox", strlen("Nitrox"))) {
        sscanf(str, "Nitrox %i", &o2);
    } else {
        sscanf(str, "%i/%i", &o2, &he);
    }

    *gas = gas_new(o2, he, MOD_AUTO);
}

void print_planhead()
{
    wprintf(L"DIVE PLAN\n\n");
    wprintf(L" %-1s  %-5s  %-8s  %-7s  %1s %-9s  %-4s  %-3s\n", "", "Depth", "Duration", "Runtime", "", "Gas", "pO2",
            "EAD");
}

void print_planline(const wchar_t sign, const double depth, const double time, const double runtime, const gas_t *gas)
{
    static char gasbuf[11];
    static char runbuf[8];
    static char pO2buf[5];
    static char eadbuf[4];
    static char timbuf[16];

    static gas_t last_gas;

    const int depth_m = round(bar_to_msw(gauge_depth(depth)));
    const int ead_m = round(bar_to_msw(max(0, gauge_depth(ead(depth, gas)))));

    wchar_t swi = L' ';

    snprintf(runbuf, len(runbuf), "(%i)", (int) ceil(runtime));
    format_gas(gasbuf, len(gasbuf), gas);
    format_mm_ss(timbuf, len(timbuf), time);

    /* print gas swich symbol if gas changed */
    if (!gas_equal(gas, &last_gas)) {
        last_gas = *gas;
        swi = SWI;
    }

    /* only print ead and pO2 on stops */
    if (sign == LVL) {
        snprintf(eadbuf, 4, "%3i", ead_m);
        snprintf(pO2buf, 5, "%4.2f", ppO2(depth, gas));
    } else {
        snprintf(eadbuf, 4, "%3s", "-");
        snprintf(pO2buf, 5, "%4s", "-");
    }

    wprintf(L" %lc  %4im  %8s  %-7s  %lc %-9s  %s  %s\n", sign, depth_m, timbuf, runbuf, swi, gasbuf, pO2buf, eadbuf);
}

void print_planfoot(const decostate_t *ds)
{
    char *model;
    char *rq;

    if (ALGO_VER == ZHL_16A)
        model = "ZHL-16A";
    else if (ALGO_VER == ZHL_16B)
        model = "ZHL-16B";
    else if (ALGO_VER == ZHL_16C)
        model = "ZHL-16C";
    else
        model = "???";

    if (P_WV == P_WV_BUHL)
        rq = "1.0";
    else if (P_WV == P_WV_NAVY)
        rq = "0.9";
    else if (P_WV == P_WV_SCHR)
        rq = "0.8";
    else
        rq = "???";

    wprintf(L"\nDeco model: Buhlmann %s\n", model);
    wprintf(L"Conservatism: GF %i/%i, Rq = %s\n", ds->gflo, ds->gfhi, rq);
    wprintf(L"Surface pressure: %4.3fbar\n\n", SURFACE_PRESSURE);

    wprintf(L"WARNING: DIVE PLAN MAY BE INACCURATE AND MAY CONTAIN\nERRORS THAT COULD LEAD TO INJURY OR DEATH.\n");
}
