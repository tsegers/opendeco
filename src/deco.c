/* SPDX-License-Identifier: MIT-0 */

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "deco.h"

#define RND(x) (round((x) *10000) / 10000)

enum ALGO ALGO_VER = ALGO_VER_DEFAULT;
double SURFACE_PRESSURE = SURFACE_PRESSURE_DEFAULT;
double P_WV = P_WV_DEFAULT;

double PO2_MAX = PO2_MAX_DEFAULT;
double END_MAX = END_MAX_DEFAULT;

int LAST_STOP_AT_SIX = LAST_STOP_AT_SIX_DEFAULT;

typedef struct zhl_n2_t {
    double t;
    double a[3];
    double b;
} zhl_n2_t;

typedef struct zhl_he_t {
    double t;
    double a;
    double b;
} zhl_he_t;

const zhl_n2_t ZHL16N[] = {
    {.t = 5.0,   .a = {1.1696, 1.1696, 1.1696}, .b = 0.5578},
    {.t = 8.0,   .a = {1.0000, 1.0000, 1.0000}, .b = 0.6514},
    {.t = 12.5,  .a = {0.8618, 0.8618, 0.8618}, .b = 0.7222},
    {.t = 18.5,  .a = {0.7562, 0.7562, 0.7562}, .b = 0.7825},
    {.t = 27.0,  .a = {0.6667, 0.6667, 0.6200}, .b = 0.8126},
    {.t = 38.3,  .a = {0.5933, 0.5600, 0.5043}, .b = 0.8434},
    {.t = 54.3,  .a = {0.5282, 0.4947, 0.4410}, .b = 0.8693},
    {.t = 77.0,  .a = {0.4701, 0.4500, 0.4000}, .b = 0.8910},
    {.t = 109.0, .a = {0.4187, 0.4187, 0.3750}, .b = 0.9092},
    {.t = 146.0, .a = {0.3798, 0.3798, 0.3500}, .b = 0.9222},
    {.t = 187.0, .a = {0.3497, 0.3497, 0.3295}, .b = 0.9319},
    {.t = 239.0, .a = {0.3223, 0.3223, 0.3065}, .b = 0.9403},
    {.t = 305.0, .a = {0.2971, 0.2850, 0.2835}, .b = 0.9477},
    {.t = 390.0, .a = {0.2737, 0.2737, 0.2610}, .b = 0.9544},
    {.t = 498.0, .a = {0.2523, 0.2523, 0.2480}, .b = 0.9602},
    {.t = 635.0, .a = {0.2327, 0.2327, 0.2327}, .b = 0.9653},
};

const zhl_he_t ZHL16He[] = {
    {.t = 1.88,   .a = 1.6189, .b = 0.4770},
    {.t = 3.02,   .a = 1.3830, .b = 0.5747},
    {.t = 4.72,   .a = 1.1919, .b = 0.6527},
    {.t = 6.99,   .a = 1.0458, .b = 0.7223},
    {.t = 10.21,  .a = 0.9220, .b = 0.7582},
    {.t = 14.48,  .a = 0.8205, .b = 0.7957},
    {.t = 20.53,  .a = 0.7305, .b = 0.8279},
    {.t = 29.11,  .a = 0.6502, .b = 0.8553},
    {.t = 41.20,  .a = 0.5950, .b = 0.8757},
    {.t = 55.19,  .a = 0.5545, .b = 0.8903},
    {.t = 70.69,  .a = 0.5333, .b = 0.8997},
    {.t = 90.34,  .a = 0.5189, .b = 0.9073},
    {.t = 115.29, .a = 0.5181, .b = 0.9122},
    {.t = 147.42, .a = 0.5176, .b = 0.9171},
    {.t = 188.24, .a = 0.5172, .b = 0.9217},
    {.t = 240.03, .a = 0.5119, .b = 0.9267},
};

double bar_to_msw(const double bar)
{
    return bar * 10;
}

double msw_to_bar(const double msw)
{
    return msw / 10;
}

double abs_depth(const double gd)
{
    return gd + SURFACE_PRESSURE;
}

double gauge_depth(const double ad)
{
    return ad - SURFACE_PRESSURE;
}

gas_t gas_new(const unsigned char o2, const unsigned char he, double mod)
{
    assert(o2 + he <= 100);

    if (mod == MOD_AUTO) {
        double mod_po2 = PO2_MAX / (o2 / 100.0);
        double mod_end = END_MAX / (1 - he / 100.0);

        mod = min(mod_po2, mod_end);
    }

    return (gas_t){.o2 = o2, .he = he, .n2 = 100 - o2 - he, .mod = mod};
}

int gas_equal(const gas_t *g1, const gas_t *g2)
{
    return g1->o2 == g2->o2 && g1->he == g2->he && g1->mod == g2->mod;
}

unsigned char gas_o2(const gas_t *gas)
{
    return gas->o2;
}

unsigned char gas_he(const gas_t *gas)
{
    return gas->he;
}

unsigned char gas_n2(const gas_t *gas)
{
    return gas->n2;
}

double gas_mod(const gas_t *gas)
{
    return gas->mod;
}

double add_segment_ascdec(decostate_t *ds, const double dstart, const double dend, const double time, const gas_t *gas)
{
    assert(time > 0);

    const double rate = (dend - dstart) / time;

    for (int i = 0; i < 16; i++) {
        double pio = gas_he(gas) / 100.0 * (dstart - P_WV);
        double po = ds->phe[i];
        double r = gas_he(gas) / 100.0 * rate;
        double k = log(2) / ZHL16He[i].t;
        double t = time;

        ds->phe[i] = pio + r * (t - 1 / k) - (pio - po - (r / k)) * exp(-k * t);
    }

    for (int i = 0; i < 16; i++) {
        double pio = gas_n2(gas) / 100.0 * (dstart - P_WV);
        double po = ds->pn2[i];
        double r = gas_n2(gas) / 100.0 * rate;
        double k = log(2) / ZHL16N[i].t;
        double t = time;

        ds->pn2[i] = pio + r * (t - 1 / k) - (pio - po - (r / k)) * exp(-k * t);
    }

    /* TODO add CNS */
    /* TODO add OTU */

    if (dend > ds->max_depth)
        ds->max_depth = dend;

    return time;
}

double add_segment_const(decostate_t *ds, const double depth, const double time, const gas_t *gas)
{
    assert(time > 0);

    for (int i = 0; i < 16; i++) {
        double pio = gas_he(gas) / 100.0 * (depth - P_WV);
        double po = ds->phe[i];
        double k = log(2) / ZHL16He[i].t;
        double t = time;

        ds->phe[i] = po + (pio - po) * (1 - exp(-k * t));
    }

    for (int i = 0; i < 16; i++) {
        double pio = gas_n2(gas) / 100.0 * (depth - P_WV);
        double po = ds->pn2[i];
        double k = log(2) / ZHL16N[i].t;
        double t = time;

        ds->pn2[i] = po + (pio - po) * (1 - exp(-k * t));
    }

    /* TODO add CNS */
    /* TODO add OTU */

    if (depth > ds->max_depth)
        ds->max_depth = depth;

    return time;
}

double get_gf(const decostate_t *ds, const double depth)
{
    const unsigned char lo = ds->gflo;
    const unsigned char hi = ds->gfhi;

    double last_stop_gauge = LAST_STOP_AT_SIX ? 2 * ds->ceil_multiple : ds->ceil_multiple;

    if (ds->firststop == -1)
        return lo;

    if (depth <= SURFACE_PRESSURE + last_stop_gauge)
        return hi;

    if (depth >= ds->firststop)
        return lo;

    /* interpolate lo and hi between first stop and last stop */
    return hi - (hi - lo) * gauge_depth(depth - ds->ceil_multiple) / gauge_depth(ds->firststop - ds->ceil_multiple);
}

double round_ceiling(const decostate_t *ds, const double c)
{
    assert(ds->ceil_multiple != 0);

    int numbered_stop = ceil(RND(gauge_depth(c) / ds->ceil_multiple));

    if (numbered_stop == 1 && LAST_STOP_AT_SIX)
        numbered_stop = 2;

    return abs_depth(ds->ceil_multiple * numbered_stop);
}

double ceiling(const decostate_t *ds, double gf)
{
    double c = 0;
    gf /= 100;

    for (int i = 0; i < 16; i++) {
        /* n2 a and b values */
        double an = ZHL16N[i].a[ALGO_VER];
        double bn = ZHL16N[i].b;

        /* he a and b values */
        double ah = ZHL16He[i].a;
        double bh = ZHL16He[i].b;

        /* scale n2 and he values for a and b proportional to their pressure */
        double pn2 = ds->pn2[i];
        double phe = ds->phe[i];

        double a = ((an * pn2) + (ah * phe)) / (pn2 + phe);
        double b = ((bn * pn2) + (bh * phe)) / (pn2 + phe);

        /* update ceiling */
        c = max(c, ((pn2 + phe) - (a * gf)) / (gf / b + 1 - gf));
    }

    return round_ceiling(ds, c);
}

double gf99(const decostate_t *ds, double depth)
{
    double gf = 0;

    for (int i = 0; i < 16; i++) {
        /* n2 a and b values */
        double an = ZHL16N[i].a[ALGO_VER];
        double bn = ZHL16N[i].b;

        /* he a and b values */
        double ah = ZHL16He[i].a;
        double bh = ZHL16He[i].b;

        /* scale n2 and he values for a and b proportional to their pressure */
        double pn2 = ds->pn2[i];
        double phe = ds->phe[i];

        double a = ((an * pn2) + (ah * phe)) / (pn2 + phe);
        double b = ((bn * pn2) + (bh * phe)) / (pn2 + phe);

        /* update gf99 */
        gf = max(gf, (pn2 + phe - depth) / (a + depth / b - depth));
    }

    return gf * 100;
}

void init_tissues(decostate_t *ds)
{
    const double pn2 = 0.79 * (SURFACE_PRESSURE - P_WV);
    const double phe = 0.00 * (SURFACE_PRESSURE - P_WV);

    for (int i = 0; i < 16; i++)
        ds->pn2[i] = pn2;

    for (int i = 0; i < 16; i++)
        ds->phe[i] = phe;
}

void init_decostate(decostate_t *ds, const unsigned char gflo, const unsigned char gfhi, const double ceil_multiple)
{
    init_tissues(ds);

    ds->gflo = gflo;
    ds->gfhi = gfhi;
    ds->firststop = -1;
    ds->max_depth = 0;
    ds->ceil_multiple = ceil_multiple;
}

double ppO2(double depth, const gas_t *gas)
{
    return gas_o2(gas) / 100.0 * depth;
}

double end(double depth, const gas_t *gas)
{
    return (gas_o2(gas) + gas_n2(gas)) / 100.0 * depth;
}

double ead(double depth, const gas_t *gas)
{
    return depth * gas_n2(gas) / 79.0;
}
