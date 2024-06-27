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

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

static ProcessUtility_hook_type prev_ProcessUtility = NULL;
static bool is_master = true;

void set_master(bool master);
bool get_master(void);

PG_FUNCTION_INFO_V1(set_master_c);
PG_FUNCTION_INFO_V1(get_master_c);

static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, 
    bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, 
    QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc);

void _PG_init(void) {
    ereport(LOG,
        (errmsg("ITS MY WORK ^_^ 2")));
    prev_ProcessUtility = ProcessUtility_hook;
    ProcessUtility_hook = my_ProcessUtility_hook; 

    if (get_master()) {
        create_publication("logical_ddl.ddl_commands");
    }

}

void _PG_fini(void) {
    ProcessUtility_hook = prev_ProcessUtility; 
}

Datum set_master_c(PG_FUNCTION_ARGS) {
    bool master = PG_GETARG_BOOL(0);
    set_master(master);
    PG_RETURN_VOID();
}


Datum get_master_c(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(get_master());
}

static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc) {
    Node *parsetree = pstmt->utilityStmt;
    const char *command_type = get_command_type(parsetree);
    const char *command_tag = get_command_tag(parsetree);

    if (prev_ProcessUtility) {
        prev_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    } else {
        standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    }
    
    if (command_type && command_tag) {
       insert_ddl_command(command_type, command_tag, queryString);
    }
}

void set_master(bool master) {
    is_master = master;
}
bool get_master(void) {
    return is_master;
}
