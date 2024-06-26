#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "commands/dbcommands.h"
#include "catalog/namespace.h"
#include "executor/spi.h"
#include "tcop/utility.h"
#include "commands/sequence.h"
#include "nodes/execnodes.h"
#include "tcop/deparse_utility.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

static ProcessUtility_hook_type prev_ProcessUtility = NULL;

void insert_ddl_command(const char *command_type, const char *command_tag, const char *command_text);
const char *get_command_type(Node *parsetree);
const char *get_command_tag(Node *parsetree);
static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, 
    bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, 
    QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc);

void _PG_init(void) {
    ereport(LOG,
        (errmsg("ITS MY WORK ^_^ 2")));
    prev_ProcessUtility = ProcessUtility_hook;
    ProcessUtility_hook = my_ProcessUtility_hook; 
}

void _PG_fini(void) {
    ProcessUtility_hook = prev_ProcessUtility; 
}

static void my_ProcessUtility_hook(PlannedStmt *pstmt, const char *queryString, bool readOnlyTree, ProcessUtilityContext context, ParamListInfo params, QueryEnvironment *queryEnv, DestReceiver *dest, QueryCompletion *qc) {
    bool success = false;
    PG_TRY();
    {
        if (prev_ProcessUtility) {
            prev_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
        } else {
            standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
        }
        success = true;
    }
    PG_CATCH();
    {
        success = false;
        PG_RE_THROW();
    }
    PG_END_TRY();
    
    if (pstmt && pstmt->utilityStmt) {
        Node *parsetree = pstmt->utilityStmt;
        const char *command_type = NULL;
        const char *command_tag = NULL;

        command_type = get_command_type(parsetree);
        command_tag = get_command_tag(parsetree);

        if (command_type && command_tag) {
            insert_ddl_command(command_type, command_tag, queryString);
        }
    }

    if (prev_ProcessUtility) {
        prev_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    } else {
        standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    }
}

const char *get_command_type(Node *parsetree) {
    switch (nodeTag(parsetree)) {
        case T_CreateStmt:
            return "Create";
        case T_AlterTableStmt:
            return "Alter";
        case T_DropStmt:
            return "Drop";
        case T_IndexStmt:
            return "Index";
        case T_GrantStmt:
            return "Grant";
        case T_GrantRoleStmt:
            return "Grant Role";
        default:
            return "Other";
    }
}

const char *get_command_tag(Node *parsetree) {
    switch (nodeTag(parsetree)) {
        case T_CreateStmt:
            return "CREATE TABLE";
        case T_AlterTableStmt:
            return "ALTER TABLE";
        case T_DropStmt:
            return "DROP TABLE";
        case T_IndexStmt:
            return "CREATE INDEX";
        case T_GrantStmt:
            return "GRANT";
        case T_GrantRoleStmt:
            return "GRANT ROLE";
        default:
            return "UNKNOWN";
    }
}

void insert_ddl_command(const char *command_type, const char *command_tag, const char *command_text) {
    int ret;
    SPI_connect();

    const char *query = "INSERT INTO logical_ddl.ddl_commands (command_type, command_tag, command_text) VALUES ($1, $2, $3)";
    Oid argtypes[3] = {TEXTOID, TEXTOID, TEXTOID};
    Datum values[3];

    values[0] = CStringGetTextDatum(command_type);
    values[1] = CStringGetTextDatum(command_tag);
    values[2] = CStringGetTextDatum(command_text);

    ret = SPI_execute_with_args(query, 3, argtypes, values, NULL, false, 0);
    if (ret != SPI_OK_INSERT) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("Failed to insert DDL command into table")));
    }

    SPI_finish();
}
