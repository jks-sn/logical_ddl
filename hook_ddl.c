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
#include "nodes/parsenodes.h"
#include "ddl_parser.h"
#include "catalog/namespace.h"  
#include "catalog/pg_namespace.h"
#include "catalog/pg_class.h"    
#include "utils/lsyscache.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

static ProcessUtility_hook_type prev_ProcessUtility = NULL;
static bool is_master;

static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, 
    bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, 
    QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc);

void DDLSender(PlannedStmt *pstmt, const char *query_String,
                                   ProcessUtilityContext context, ParamListInfo params,
                                   QueryEnvironment *queryEnv, DestReceiver *dest,
                                   QueryCompletion *qc);

void DDLReceiver(PlannedStmt *pstmt, const char *query_String,
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
    int			stmt_start = pstmt->stmt_location > 0 ? pstmt->stmt_location : 0;
	int			stmt_len = pstmt->stmt_len > 0 ? pstmt->stmt_len : strlen(queryString + stmt_start);
	char	   *query_string = palloc(stmt_len + 1);
	strncpy(query_string, queryString + stmt_start, stmt_len);
	query_string[stmt_len] = 0;
    ereport(LOG, (errmsg("my_ProcessUtility_hook called for command: %s", query_string)));
    if (is_master) {
        DDLSender(pstmt, query_string, context, params, queryEnv, dest, qc);
    } else {
        DDLReceiver(pstmt, query_string, context, params, queryEnv, dest, qc);
    }
    pfree(query_string);
    if (prev_ProcessUtility) {
        prev_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    } else {
        standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    }
}

void DDLSender(PlannedStmt *pstmt, const char *query_string,
                            ProcessUtilityContext context, ParamListInfo params,
                            QueryEnvironment *queryEnv, DestReceiver *dest,
                            QueryCompletion *qc) {
    Node *parsetree = pstmt->utilityStmt;
    const char *command_type = NULL;
    const char *command_tag = NULL;
    const char *command_schema_name = NULL;
    const char *command_relation_name = NULL;
    bool do_command = true;
    switch (nodeTag(parsetree)) {       
        case T_CreateStmt: {
            CreateStmt *stmt = (CreateStmt *) parsetree;
            command_type = "Create";
            command_tag = "CREATE TABLE";
            command_schema_name = stmt->relation->schemaname;
            command_relation_name = stmt->relation->relname;
            ereport(LOG, (errmsg("CreateStmt detected: schema=%s, relation=%s", command_schema_name, command_relation_name)));
            break;
        }
        case T_AlterTableStmt: {
            AlterTableStmt *stmt = (AlterTableStmt *) parsetree;
            command_type = "Alter";
            command_tag = "ALTER TABLE";
            command_schema_name = stmt->relation->schemaname;
            command_relation_name = stmt->relation->relname;
            break;
        }
        case T_DropStmt: {
            DropStmt *stmt = (DropStmt *) parsetree;
            command_type = "Drop";
            command_tag = "DROP TABLE";
            if (stmt->removeType == OBJECT_TABLE) {
                ListCell *cell;
                foreach (cell, stmt->objects) {
                    List *objName = (List *)lfirst(cell);
                    command_relation_name = strVal(linitial(objName));
                    Oid relid = get_relname_relid(command_relation_name, InvalidOid);
                    if (OidIsValid(relid)) {
                        Oid namespace_id = get_rel_namespace(relid);
                        command_schema_name = get_namespace_name(namespace_id);
                    }
                }
            }
            else {
                do_command = false;
            }
            break;
        }
        default: {
            do_command = false;
            ereport(LOG, (errmsg("Skipping insertion of DDL command due to NULL command_type or command_tag with query=%s", query_string)));
            break;
        }
    }
    if (command_schema_name == NULL && command_relation_name != NULL) {
        Oid relid = get_relname_relid(command_relation_name, InvalidOid);
        if (OidIsValid(relid)) {
            Oid namespace_id = get_rel_namespace(relid);
            command_schema_name = get_namespace_name(namespace_id);
            ereport(LOG, (errmsg("Fallback schema name extraction: schema=%s, relation=%s", command_schema_name, command_relation_name)));
        }
    }
    if(do_command) { 
        ereport(LOG, (errmsg("Handling command with type: %s, tag: %s, schema: %s, relation: %s, query: %s", command_type, command_tag, command_schema_name, command_relation_name, query_string)));
        insert_ddl_command(command_type, command_tag, command_schema_name, command_relation_name, query_string);
    }
}

void DDLReceiver(PlannedStmt *pstmt, const char *query_string,
                              ProcessUtilityContext context, ParamListInfo params,
                              QueryEnvironment *queryEnv, DestReceiver *dest,
                              QueryCompletion *qc) {

}


// void create_replication_slot_and_subscription(void) {
//     if (!IsTransactionState()) {
//         StartTransactionCommand();
//     }

//     SPI_connect();

//     if (is_master) {
//         ereport(LOG, (errmsg("Creating publication and replication slot on master")));

//         SPI_execute("CREATE PUBLICATION logical_ddl_pub FOR TABLE logical_ddl.ddl_commands;", false, 0);

//         SPI_execute("CREATE SUBSCRIPTION logical_ddl_status_sub" 
//                     "CONNECTION 'host=localhost port=5433 dbname=postgres'"
//                     "PUBLICATION logical_ddl_status_pub WITH (slot_name = 'false');", false, 0);
//     }
//     else {
//         ereport(LOG, (errmsg("Creating publication and subscription on replica")));

//         SPI_execute("CREATE PUBLICATION logical_ddl_status_pub FOR TABLE logical_ddl.command_status;", false, 0);

//         SPI_execute("CREATE SUBSCRIPTION logical_ddl_sub "
//                     "CONNECTION 'host=localhost port=5432 dbname=postgres'"
//                     "PUBLICATION logical_ddl_pub WITH (slot_name = 'logical_ddl_sub_slot');", false, 0);    
//     }

//     SPI_finish();
//     if (IsTransactionState())
//         CommitTransactionCommand();
// }
