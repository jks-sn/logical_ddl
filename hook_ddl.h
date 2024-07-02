#ifndef HOOK_DDL_H
#define HOOK_DDL_H

#include "postgres.h"
#include "fmgr.h"

void set_master_internal(bool master);
bool get_master_internal(void);

PG_FUNCTION_INFO_V1(set_master);
PG_FUNCTION_INFO_V1(get_master);

#endif /* HOOK_DDL_H */
