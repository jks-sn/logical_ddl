#include "ddl_parser.h"
#include "fmgr.h"

const char *get_command_type(Node *parsetree) {

    ereport(LOG, (errmsg("get_command_type nodeTag: %u", nodeTag(parsetree))));
    
    switch (nodeTag(parsetree)) {
        case T_CreateStmt:
            return "Create";
        case T_AlterTableStmt:
            return "Alter";
        case T_DropStmt:
            return "Drop";
        case T_IndexStmt:
            return "Index";
        case T_RuleStmt:
            return "Rule";
        case T_ViewStmt:
            return "View";
        case T_CreateSeqStmt:
            return "Create Sequence";
        case T_AlterSeqStmt:
            return "Alter Sequence";
        case T_VariableSetStmt:
            return "Set";
        case T_CreateTrigStmt:
            return "Create Trigger";
        case T_CreatePLangStmt:
            return "Create Language";
        case T_CreateDomainStmt:
            return "Create Domain";
        case T_CreateOpClassStmt:
            return "Create Operator Class";
        case T_CreateOpFamilyStmt:
            return "Create Operator Family";
        case T_AlterOpFamilyStmt:
            return "Alter Operator Family";
        case T_AlterObjectSchemaStmt:
            return "Alter Schema";
        case T_AlterOwnerStmt:
            return "Alter Owner";
        case T_RenameStmt:
            return "Rename";
        case T_AlterDefaultPrivilegesStmt:
            return "Alter Default Privileges";
        case T_DefineStmt:
            return "Define";
        case T_CompositeTypeStmt:
            return "Create Type";
        case T_CreateEnumStmt:
            return "Create Type";
        case T_CreateRangeStmt:
            return "Create Type";
        case T_AlterEnumStmt:
            return "Alter Type";
        case T_AlterTSDictionaryStmt:
            return "Alter Text Search Dictionary";
        case T_AlterTSConfigurationStmt:
            return "Alter Text Search Configuration";
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
        case T_RuleStmt:
            return "CREATE RULE";
        case T_ViewStmt:
            return "CREATE VIEW";
        case T_CreateSeqStmt:
            return "CREATE SEQUENCE";
        case T_AlterSeqStmt:
            return "ALTER SEQUENCE";
        case T_VariableSetStmt:
            return "SET";
        case T_CreateTrigStmt:
            return "CREATE TRIGGER";
        case T_CreatePLangStmt:
            return "CREATE LANGUAGE";
        case T_CreateDomainStmt:
            return "CREATE DOMAIN";
        case T_CreateOpClassStmt:
            return "CREATE OPERATOR CLASS";
        case T_CreateOpFamilyStmt:
            return "CREATE OPERATOR FAMILY";
        case T_AlterOpFamilyStmt:
            return "ALTER OPERATOR FAMILY";
        case T_AlterObjectSchemaStmt:
            return "ALTER SCHEMA";
        case T_AlterOwnerStmt:
            return "ALTER OWNER";
        case T_RenameStmt:
            return "RENAME";
        case T_AlterDefaultPrivilegesStmt:
            return "ALTER DEFAULT PRIVILEGES";
        case T_DefineStmt:
            return "DEFINE";
        case T_CompositeTypeStmt:
            return "CREATE TYPE";
        case T_CreateEnumStmt:
            return "CREATE TYPE";
        case T_CreateRangeStmt:
            return "CREATE TYPE";
        case T_AlterEnumStmt:
            return "ALTER TYPE";
        case T_AlterTSDictionaryStmt:
            return "ALTER TEXT SEARCH DICTIONARY";
        case T_AlterTSConfigurationStmt:
            return "ALTER TEXT SEARCH CONFIGURATION";
        default:
            return "UNKNOWN";
    }
}