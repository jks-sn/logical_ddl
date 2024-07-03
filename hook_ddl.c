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
static bool is_master;

static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, 
    bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, 
    QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc);

void DDLSender(PlannedStmt *pstmt, const char *queryString,
                                   ProcessUtilityContext context, ParamListInfo params,
                                   QueryEnvironment *queryEnv, DestReceiver *dest,
                                   QueryCompletion *qc);

void DDLReceiver(PlannedStmt *pstmt, const char *queryString,
                                     ProcessUtilityContext context, ParamListInfo params,
                                     QueryEnvironment *queryEnv, DestReceiver *dest,
                                     QueryCompletion *qc);

void _PG_init(void) {
    ereport(LOG,
        (errmsg("ITS MY WORK ^_^ 2")));

    DefineCustomBoolVariable(
        "logical_ddl.is_master",
        "Determines if this node is a master",
        NULL,
        &is_master,
        false,
        PGC_USERSET,
        0,
        NULL,
        NULL,
        NULL
    );

    ereport(LOG, (errmsg("Initializing logical_ddl extension")));
    ereport(LOG, (errmsg("logical_ddl.is_master = %s", is_master ? "true" : "false")));
    prev_ProcessUtility = ProcessUtility_hook;
    ProcessUtility_hook = my_ProcessUtility_hook; 
}

void _PG_fini(void) {
    ereport(LOG, (errmsg("Unloading logical_ddl extension")));
    ProcessUtility_hook = prev_ProcessUtility; 
}


static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc) {
    ereport(LOG, (errmsg("my_ProcessUtility_hook called for command: %s", queryString)));
    if (is_master) {
        DDLSender(pstmt, queryString, context, params, queryEnv, dest, qc);
    } else {
        DDLReceiver(pstmt, queryString, context, params, queryEnv, dest, qc);
    }

    if (prev_ProcessUtility) {
        prev_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    } else {
        standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    }
}

void DDLSender(PlannedStmt *pstmt, const char *queryString,
                            ProcessUtilityContext context, ParamListInfo params,
                            QueryEnvironment *queryEnv, DestReceiver *dest,
                            QueryCompletion *qc) {
    Node *parsetree = pstmt->utilityStmt;
    int			stmt_start = pstmt->stmt_location > 0 ? pstmt->stmt_location : 0;
	int			stmt_len = pstmt->stmt_len > 0 ? pstmt->stmt_len : strlen(queryString + stmt_start);
	char	   *query_string = palloc(stmt_len + 1);
    const char *command_type = get_command_type(parsetree);
    const char *command_tag = get_command_tag(parsetree);
    bool do_command = true;
	strncpy(query_string, queryString + stmt_start, stmt_len);
	query_string[stmt_len] = 0;

    ereport(LOG, (errmsg("Replicating DDL command from sender: %s", query_string)));
    switch (nodeTag(parsetree)) {       
        case T_CreateStmt: {
            CreateStmt *stmt = (CreateStmt *) parsetree;
            ereport(LOG, (errmsg("Handling CREATE TABLE command: %s", stmt->relation->relname)));
            break;
        }
        case T_AlterTableStmt: {
            AlterTableStmt *stmt = (AlterTableStmt *) parsetree;
            ereport(LOG, (errmsg("Handling ALTER TABLE command: %s", stmt->relation->relname)));
            break;
        }
        case T_DropStmt: {
            DropStmt *stmt = (DropStmt *) parsetree;
            if (stmt->removeType == OBJECT_TABLE) {
                ereport(LOG, (errmsg("Handling DROP TABLE command")));
            }
            break;
        }
        default: {
            do_command = false;
            ereport(LOG, (errmsg("Skipping insertion of DDL command due to NULL command_type or command_tag with query=%s", query_string)));
            break;
        }
    }
    if(do_command) {
        insert_ddl_command(command_type, command_tag, query_string);
    }
    pfree(query_string);
}

void DDLReceiver(PlannedStmt *pstmt, const char *queryString,
                              ProcessUtilityContext context, ParamListInfo params,
                              QueryEnvironment *queryEnv, DestReceiver *dest,
                              QueryCompletion *qc) {

}
