#include "postgres.h"
#include "nodes/parsenodes.h"
#include "tcop/utility.h"
#include "executor/spi.h"
#include "commands/trigger.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "ddl_commands.h"
#include "lib/stringinfo.h"

PG_FUNCTION_INFO_V1(ddl_command_trigger);

void insert_ddl_command(const char *command_type, const char *command_tag, const char *schema_name, const char *relation_name, const char *query_string) {
    int ret;
    const char *query = "INSERT INTO logical_ddl.ddl_commands (command_type, command_tag, schema_name, relation_name, command_text) VALUES ($1, $2, $3, $4, $5) RETURNING id";
    Oid argtypes[5] = {TEXTOID, TEXTOID, TEXTOID, TEXTOID, TEXTOID};
    Datum values[5];
    char nulls[5] = {' ', ' ', ' ', ' ', ' '};

    ereport(LOG, (errmsg("Inserting DDL command: type=%s, tag=%s, schema=%s, relation=%s, query=%s", 
                         command_type, command_tag, schema_name, relation_name, query_string)));

    SPI_connect();

    values[0] = CStringGetTextDatum(command_type);
    values[1] = CStringGetTextDatum(command_tag);
    values[2] = schema_name ? CStringGetTextDatum(schema_name) : (Datum) 0;
    values[3] = relation_name ? CStringGetTextDatum(relation_name) : (Datum) 0;
    values[4] = CStringGetTextDatum(query_string);

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

void execute_ddl_command(int32 id, const char *command_text) {
    const char *update_query = "UPDATE logical_ddl.ddl_commands SET executed_at = now() WHERE id = $1";
    Datum values[1] = {Int32GetDatum(id)};
    Oid argtypes[1] = {INT4OID};
    
    SPI_connect();

    SPI_execute(command_text, false, 0);

    SPI_execute_with_args(update_query, 1, argtypes, values, NULL, false, 0);

    SPI_finish();
}

void handle_ddl_command(const char *command_type, const char *command_tag, const char *command_text, const char *schema_name, const char *relation_name) {
    ereport(LOG, (errmsg("Subscriber: Handling DDL command: type=%s, tag=%s, schema=%s, relation=%s, query=%s",
                         command_type, command_tag, schema_name, relation_name, command_text)));

    SPI_connect();
    int ret = SPI_execute(command_text, false, 0);
    if (ret != SPI_OK_UTILITY && ret != SPI_OK_SELECT) {
        ereport(LOG, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("Failed to execute DDL command")));
    }
    SPI_finish();
}

Datum ddl_command_trigger(PG_FUNCTION_ARGS) {

    ereport(LOG, (errmsg("ddl_command_trigger start execute")));

    TriggerData *trigdata = (TriggerData *) fcinfo->context;
    if (!CALLED_AS_TRIGGER(fcinfo))
        ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("not called by trigger manager")));

    ereport(LOG, (errmsg("Trigger fired by event: %u", trigdata->tg_event)));

    if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event)) {
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

        handle_ddl_command(command_type, command_tag, command_text, schema_name, relation_name);

        ereport(LOG, (errmsg("ddl_command_trigger executed")));
    } else {
        ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("Trigger not fired by insert")));
    }

    return PointerGetDatum(trigdata->tg_trigtuple);
}