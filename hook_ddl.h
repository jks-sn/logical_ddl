#ifndef HOOK_DDL_H
#define HOOK_DDL_H

#include "postgres.h"
#include "fmgr.h"

void set_master_internal(bool master);
bool get_master_internal(void);

PGDLLEXPORT Datum set_master_c(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum get_master_c(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(set_master_c);
PG_FUNCTION_INFO_V1(get_master_c);

#endif /* HOOK_DDL_H */
