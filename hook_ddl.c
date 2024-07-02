#include "postgres.h"
#include "fmgr.h"
#include "commands/dbcommands.h"
#include "executor/spi.h"
#include "tcop/utility.h"
#include "nodes/execnodes.h"
#include "tcop/deparse_utility.h"
#include "miscadmin.h"
#include "storage/ipc.h"
#include "storage/proc.h"
#include "utils/builtins.h"
#include "ddl_commands.h"
#include "publication.h"
#include "subscription.h"
#include "ddl_parser.h"
#include "hook_ddl.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

static ProcessUtility_hook_type prev_ProcessUtility = NULL;
static bool is_master = true;

void set_master_internal(bool master);
bool get_master_internal(void);

static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, 
    bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, 
    QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc);

void _PG_init(void) {
    ereport(LOG, (errmsg("Initializing logical_ddl extension")));
    ereport(LOG,
        (errmsg("ITS MY WORK ^_^ 2")));
    prev_ProcessUtility = ProcessUtility_hook;
    ProcessUtility_hook = my_ProcessUtility_hook; 

    if (get_master_internal()) {
        create_publication("logical_ddl.ddl_commands");
    }

}

void _PG_fini(void) {
    ereport(LOG, (errmsg("Unloading logical_ddl extension")));
    ProcessUtility_hook = prev_ProcessUtility; 
}


static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc) {
    ereport(LOG, (errmsg("my_ProcessUtility_hook called for command: %s", queryString)));

    Node *parsetree = pstmt->utilityStmt;
    const char *command_type = get_command_type(parsetree);
    const char *command_tag = get_command_tag(parsetree);
    

    if (prev_ProcessUtility) {
        prev_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    } else {
        standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    }
    
    if (is_ddl_command(parsetree)) {
            ereport(LOG, (errmsg("Inserting DDL command: type=%s, tag=%s, query=%s", command_type, command_tag, queryString)));
            insert_ddl_command(command_type, command_tag, queryString);
    } else {
        ereport(LOG, (errmsg("Skipping insertion of DDL command due to NULL command_type or command_tag with query=%s", queryString)));
    }
}

void set_master_internal(bool master) {
    ereport(LOG, (errmsg("set_master_internal called with value: %d", master)));
    is_master = master;
}
bool get_master_internal(void) {
    ereport(LOG, (errmsg("get_master_internal called, returning value: %d", is_master)));
    return is_master;
}

Datum set_master(PG_FUNCTION_ARGS) {
    bool master = PG_GETARG_BOOL(0);
    set_master_internal(master);
    PG_RETURN_VOID();
}

Datum get_master(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(get_master_internal());
}
