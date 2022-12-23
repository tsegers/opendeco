#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "opendeco-conf.h"
#include "../toml/toml.h"

int opendeco_conf_parse(const char *confpath, struct arguments *arguments)
{
    FILE *fp;
    char errbuf[200];

    /* open config file */
    fp = fopen(confpath, "r");

    if (!fp)
        return -1;

    /* parse config */
    toml_table_t *od_conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!od_conf)
        return -1;

    fwprintf(stderr, L"Picked up options from %s\n", confpath);

    /* set options in arguments */
    toml_table_t *dive = toml_table_in(od_conf, "dive");

    if (dive) {
        toml_datum_t g = toml_string_in(dive, "gas");

        if (g.ok) {
            if (arguments->gas)
                free(arguments->gas);

            arguments->gas = g.u.s;
        }

        toml_datum_t p = toml_double_in(dive, "surface_pressure");

        if (p.ok)
            arguments->SURFACE_PRESSURE = p.u.d;

        toml_datum_t r = toml_double_in(dive, "rmv");

        if (r.ok)
            arguments->RMV_DIVE = r.u.d;
    }

    toml_table_t *deco = toml_table_in(od_conf, "deco");

    if (deco) {
        toml_datum_t L = toml_int_in(deco, "gflow");

        if (L.ok)
            arguments->gflow = L.u.i;

        toml_datum_t H = toml_int_in(deco, "gfhigh");

        if (H.ok)
            arguments->gfhigh = H.u.i;

        toml_datum_t G = toml_string_in(deco, "decogasses");

        if (G.ok) {
            if (arguments->decogasses)
                free(arguments->decogasses);

            arguments->decogasses = G.u.s;
        }

        toml_datum_t _6 = toml_bool_in(deco, "last_stop_at_six");

        if (_6.ok)
            arguments->LAST_STOP_AT_SIX = _6.u.b;

        toml_datum_t S = toml_bool_in(deco, "switch_intermediate");

        if (S.ok)
            arguments->SWITCH_INTERMEDIATE = S.u.b;

        toml_datum_t R = toml_double_in(deco, "rmv");

        if (R.ok)
            arguments->RMV_DECO = R.u.d;
    }

    toml_table_t *conf = toml_table_in(od_conf, "conf");

    if (conf) {
        toml_datum_t T = toml_bool_in(conf, "show_travel");

        if (T.ok)
            arguments->SHOW_TRAVEL = T.u.b;
    }

    toml_free(od_conf);
    return 0;
}
