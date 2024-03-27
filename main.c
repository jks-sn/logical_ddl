#include "postgres.h"
#include "fmgr.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_ddl_catch_query);

Datum pg_ddl_catch_query(PG_FUNCTION_ARGS) {
    if (debug_query_string)
    {
        ereport(LOG, errmsg("parse query string : %s", debug_query_string));
        PG_RETURN_TEXT_P(cstring_to_text(debug_query_string))
        /* do something */
    }
    else {
        PG_RETURN_NULL();
    }
}