#include "postgres.h"
#include "nodes/nodes.h"
#include "nodes/parsenodes.h"
#include "ddl_parser.h"

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