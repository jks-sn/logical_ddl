#include "postgres.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "commands/event_trigger.h"
#include "commands/dbcommands.h"
#include "miscadmin.h"
#include "storage/ipc.h"
#include "storage/proc.h"
#include "utils/builtins.h"
#include "nodes/parsenodes.h"
#include "catalog/pg_event_trigger.h"
#include "parser/analyze.h"
#include "parser/parser.h"
#include "utils/guc.h"
#include "ddl_commands.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

static bool is_master;

static void DDLSender(Oid classid, Oid objid, int32 objsubid, const char *commandTag,
                      const char *objectType, const char *schemaName,
                      const char *objectIdentity, const char *commandText);

static void DDLReceiver(Oid classid, Oid objid, int32 objsubid, const char *commandTag, 
                        const char *objectType, const char *schemaName, 
                        const char *objectIdentity, const char *commandText);


PG_FUNCTION_INFO_V1(handle_ddl_command_c);

Datum
handle_ddl_command_c(PG_FUNCTION_ARGS) {
    Oid classid = PG_GETARG_OID(0);
    Oid objid = PG_GETARG_OID(1);
    int32 objsubid = PG_GETARG_INT32(2);
    const char *commandTag = text_to_cstring(PG_GETARG_TEXT_P(3));
    const char *objectType = text_to_cstring(PG_GETARG_TEXT_P(4));
    const char *schemaName = PG_ARGISNULL(5) ? NULL : text_to_cstring(PG_GETARG_TEXT_P(5));
    const char *objectIdentity = text_to_cstring(PG_GETARG_TEXT_P(6));
    const char *commandText = text_to_cstring(PG_GETARG_TEXT_P(7));

    if (is_master)
    {
        DDLSender(classid, objid, objsubid, commandTag, objectType, schemaName, objectIdentity, commandText);
    }
    else
    {
        DDLReceiver(classid, objid, objsubid, commandTag, objectType, schemaName, objectIdentity, commandText);
    }

    PG_RETURN_VOID();
}

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

}

void _PG_fini(void) {
    ereport(LOG, (errmsg("Unloading logical_ddl extension")));

}



static void DDLSender(Oid classid, Oid objid, int32 objsubid, const char *commandTag, const char *objectType, const char *schemaName, const char *objectIdentity, const char *commandText) {
    bool to_replicate = false;

    ereport(LOG,
            (errmsg("Handling DDL command: classid=%u, objid=%u, objsubid=%d, tag=%s, type=%s, schema=%s, identity=%s, query=%s",
                    classid, objid, objsubid, commandTag, objectType, schemaName ? schemaName : "(null)", objectIdentity, commandText)));

    if (strcmp(commandTag, "CREATE TABLE") == 0)
    {
        to_replicate = true;
    }
    else if (strcmp(commandTag, "ALTER TABLE") == 0)
    {
        to_replicate = true;
    }
    else if (strcmp(commandTag, "DROP TABLE") == 0)
    {
        to_replicate = true;
    }
    else
    {
        ereport(LOG, (errmsg("Skipping DDL command: %s", commandTag)));
    }

    if (to_replicate)
    {
        insert_ddl_command(classid, objid, objsubid, commandTag, objectType, schemaName, objectIdentity, commandText);
    }
}

static void DDLReceiver(Oid classid, Oid objid, int32 objsubid, const char *commandTag, const char *objectType, const char *schemaName, const char *objectIdentity, const char *commandText) {
    ereport(LOG,
            (errmsg("Received DDL command: classid=%u, objid=%u, objsubid=%d, tag=%s, type=%s, schema=%s, identity=%s, query=%s",
                    classid, objid, objsubid, commandTag, objectType, schemaName ? schemaName : "(null)", objectIdentity, commandText)));
}
