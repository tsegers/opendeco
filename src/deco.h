/* SPDX-License-Identifier: MIT-0 */

#ifndef DECO_H
#define DECO_H

#include <stddef.h>

#define max(X, Y) (((X) > (Y)) ? (X) : (Y))
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define len(X) (sizeof(X) / sizeof((X)[0]))

#define P_WV_BUHL 0.0627 /* Buhlmann value, Rq = 1.0, least conservative */
#define P_WV_NAVY 0.0567 /* US. Navy value, Rq = 0.9 */
#define P_WV_SCHR 0.0493 /* Schreiner value, Rq = 0.8, most conservative */

#define ALGO_VER_DEFAULT ZHL_16C
#define SURFACE_PRESSURE_DEFAULT 1.01325
#define P_WV_DEFAULT P_WV_BUHL

#define PO2_MAX_DEFAULT 1.6
#define END_MAX_DEFAULT 4.01325

#define LAST_STOP_AT_SIX_DEFAULT 0

#define MOD_AUTO 0

/* types */
enum ALGO {
    ZHL_16A = 0,
    ZHL_16B = 1,
    ZHL_16C = 2,
};

typedef struct decostate_t {
    double pn2[16];
    double phe[16];
    unsigned char gflo;
    unsigned char gfhi;
    double firststop;
    double max_depth;
    double ceil_multiple;
} decostate_t;

typedef struct gas_t {
    unsigned char o2;
    unsigned char he;
    unsigned char n2;
    double mod;
} gas_t;

/* global variables */
extern enum ALGO ALGO_VER;
extern double SURFACE_PRESSURE;
extern double P_WV;

extern double PO2_MAX;
extern double END_MAX;

extern int LAST_STOP_AT_SIX;

/* functions */
double bar_to_msw(const double bar);
double msw_to_bar(const double msw);
double abs_depth(const double gd);
double gauge_depth(const double ad);

gas_t gas_new(const unsigned char o2, const unsigned char he, double mod);
int gas_equal(const gas_t *g1, const gas_t *g2);
unsigned char gas_o2(const gas_t *gas);
unsigned char gas_he(const gas_t *gas);
unsigned char gas_n2(const gas_t *gas);
double gas_mod(const gas_t *gas);

double add_segment_ascdec(decostate_t *ds, const double dstart, const double dend, const double time,
                          const gas_t *gas);
double add_segment_const(decostate_t *ds, const double depth, const double time, const gas_t *gas);
double get_gf(const decostate_t *ds, const double depth);
double ceiling(const decostate_t *ds, double gf);
double gf99(const decostate_t *ds, double depth);

void init_decostate(decostate_t *ds, const unsigned char gflo, const unsigned char gfhi, const double ceil_multiple);

double ppO2(double depth, const gas_t *gas);
double end(double depth, const gas_t *gas);
double ead(double depth, const gas_t *gas);

#endif /* end of include guard: DECO_H */
