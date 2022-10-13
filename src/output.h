/* SPDX-License-Identifier: MIT-0 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <wchar.h>

#include "deco.h"

#define ASC 0x2197 /* Unicode North East Arrow */
#define LVL 0x2192 /* Unicode Rightwards Arrow */
#define DEC 0x2198 /* Unicode South East Arrow */
#define SWI 0x21BB /* Clockwise Open Circle Arrow */

/* functions */
void print_planhead(void);
void print_planline(const wchar_t sign, const double depth, const double time, const double runtime, const gas_t *gas);
void print_planfoot(const decostate_t *ds);

void scan_gas(gas_t *gas, char *str);

#endif /* end of include guard: OUTPUT_H */
