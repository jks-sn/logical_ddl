#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "ddl_commands.h"
#include "executor/spi.h"
#include "nodes/parsenodes.h"
#include "tcop/utility.h"
#include "commands/trigger.h"
#include "lib/stringinfo.h"
#include "utils/builtins.h"
#include "utils/rel.h"
#include "utils/elog.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"
#include "access/xlog.h"
#include "access/xact.h"

void insert_ddl_command(const char *command_type, const char *command_tag, const char *schema_name, const char *relation_name, const char *query_string) {
    int ret;
    const char *query = "INSERT INTO logical_ddl.ddl_commands (command_type, command_tag, schema_name, relation_name, command_text) VALUES ($1, $2, $3, $4, $5) RETURNING id";
    Oid argtypes[5] = {TEXTOID, TEXTOID, TEXTOID, TEXTOID, TEXTOID};
    Datum values[5];
    char nulls[5] = {' ', ' ', ' ', ' ', ' '};

    ereport(LOG, (errmsg("Inserting DDL command: type=%s, tag=%s, schema=%s, relation=%s, query=%s", 
                         command_type, command_tag, schema_name, relation_name, query_string)));

    SPI_connect();

    values[0] = command_type ? CStringGetTextDatum(command_type) : (Datum) 0;
    values[1] = command_tag ? CStringGetTextDatum(command_tag) : (Datum) 0;
    values[2] = schema_name ? CStringGetTextDatum(schema_name) : (Datum) 0;
    values[3] = relation_name ? CStringGetTextDatum(relation_name) : (Datum) 0;
    values[4] =  query_string ? CStringGetTextDatum(query_string) : (Datum) 0;

    if (schema_name == NULL) {
        nulls[2] = 'n';
    }

    if (relation_name == NULL) {
        nulls[3] = 'n';
    }

    ret = SPI_execute_with_args(query, 5, argtypes, values, nulls, false, 0);

    if (ret != SPI_OK_INSERT_RETURNING) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("Failed to insert DDL command into table")));
    }
    SPI_finish();
}

PG_FUNCTION_INFO_V1(ddl_command_trigger);
Datum ddl_command_trigger(PG_FUNCTION_ARGS) {
    ereport(LOG, (errmsg("ddl_command_trigger start execute")));

    TriggerData *trigdata = (TriggerData *) fcinfo->context;
    if (!CALLED_AS_TRIGGER(fcinfo))
        ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("not called by trigger manager")));

    ereport(LOG, (errmsg("Trigger fired by event: %u", trigdata->tg_event)));

    if (!TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
        ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("not fired by an insert")));

    TupleDesc tupdesc = trigdata->tg_relation->rd_att;
    HeapTuple rettuple = trigdata->tg_trigtuple;

    char *id = SPI_getvalue(rettuple, tupdesc, 1);
    char *command_type = SPI_getvalue(rettuple, tupdesc, 2);
    char *command_tag = SPI_getvalue(rettuple, tupdesc, 3);
    char *schema_name = SPI_getvalue(rettuple, tupdesc, 4);
    char *relation_name = SPI_getvalue(rettuple, tupdesc, 5);
    char *command_text = SPI_getvalue(rettuple, tupdesc, 6);

    ereport(LOG, (errmsg("Received DDL command: type=%s, tag=%s, schema=%s, relation=%s, text=%s",
                            command_type ? command_type : "(null)",
                            command_tag ? command_tag : "(null)",
                            schema_name ? schema_name : "(null)",
                            relation_name ? relation_name : "(null)",
                            command_text ? command_text : "(null)")));

    bool command_success = false;
    char *error_message = NULL;
    
	MemoryContext oldcontext = CurrentMemoryContext;
    PG_TRY(); 
    {
        SPI_connect();
        int ret = SPI_execute(command_text, false, 0);

        if (ret != SPI_OK_UTILITY) {
            ereport(ERROR, (errmsg("Failed to execute command: %s", command_text)));
        }
        ereport(LOG, (errmsg("Successfully executed command: %s", command_text)));
        command_success = true;
    }
    PG_CATCH();
    {
        ErrorData *edata;

        MemoryContextSwitchTo(oldcontext);
        edata = CopyErrorData();
        FlushErrorState();

        ereport(LOG, (errmsg("Failed to execute command: %s, Error: %s", command_text, edata->message)));
        error_message = pstrdup(edata->message); //problems with pallloc ?
        command_success = false;
    }
    PG_END_TRY();
    
    SPI_finish();
    
    SPI_connect();
    const char *query = "INSERT INTO logical_ddl.command_status (command_id, status, result, error_message) VALUES ($1, $2, $3, $4)";
    Oid argtypes[4] = {INT4OID, TEXTOID, TEXTOID, TEXTOID};
    Datum values[4];
    char nulls[4] = {' ', ' ', 'n', 'n'};

    values[0] = Int32GetDatum(atoi(id));
    values[1] = CStringGetTextDatum(command_success ? "success" : "failure");

    if (command_success) {
        values[2] = CStringGetTextDatum("Command executed successfully");
        nulls[2] = false;
    } else if (error_message) {
        values[3] = CStringGetTextDatum(error_message);
        nulls[3] = false;
    }

    SPI_execute_with_args(query, 4, argtypes, values, nulls, false, 0);
    SPI_finish();

    ereport(LOG, (errmsg("ddl_command_trigger executed")));

    PG_RETURN_POINTER(rettuple);
}

PG_FUNCTION_INFO_V1(command_status_trigger);
Datum command_status_trigger(PG_FUNCTION_ARGS) {
    TriggerData *trigdata = (TriggerData *) fcinfo->context;
    if (!CALLED_AS_TRIGGER(fcinfo))
        ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("not called by trigger manager")));
    if (!TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
        ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("not fired by an insert")));

    HeapTuple rettuple = trigdata->tg_trigtuple;
    TupleDesc tupdesc = trigdata->tg_relation->rd_att;

    char *command_id = SPI_getvalue(rettuple, tupdesc, 2);
    char *status = SPI_getvalue(rettuple, tupdesc, 3);
    char *error_message = SPI_getvalue(rettuple, tupdesc, 5);

    if (strcmp(status, "failure") == 0) {
        ereport(NOTICE, (errmsg("Error received from replica for command ID %s: %s", command_id, error_message)));
    } else {
        ereport(NOTICE, (errmsg("Command ID %s executed successfully on replica.", command_id)));
    }

    return PointerGetDatum(trigdata->tg_trigtuple);
}
