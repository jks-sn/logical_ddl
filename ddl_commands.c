#include "postgres.h"
#include "executor/spi.h"
#include "commands/trigger.h"
#include "utils/builtins.h"
#include "ddl_commands.h"

PG_FUNCTION_INFO_V1(ddl_command_trigger);

void insert_ddl_command(const char *command_type, const char *command_tag, const char *command_text) {
    int ret;
    const char *query = "INSERT INTO logical_ddl.ddl_commands (command_type, command_text) VALUES ($1, $2) RETURNING id";
    Oid argtypes[3] = {TEXTOID, TEXTOID, TEXTOID};
    Datum values[3];
    
    SPI_connect();

    values[0] = CStringGetTextDatum(command_type);
    values[1] = CStringGetTextDatum(command_tag);
    values[2] = CStringGetTextDatum(command_text);

    ret = SPI_execute_with_args(query, 3, argtypes, values, NULL, false, 0);
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

Datum ddl_command_trigger(PG_FUNCTION_ARGS) {
    TriggerData *trigdata = (TriggerData *) fcinfo->context;
    if (!CALLED_AS_TRIGGER(fcinfo)) {
        ereport(ERROR,
                (errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
                 errmsg("ddl_command_trigger: called, but not by trigger")));
    }

    if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event)) {
        HeapTuple new_row = trigdata->tg_trigtuple;
        bool isnull;
        Datum id_datum = SPI_getbinval(new_row, trigdata->tg_relation->rd_att, SPI_fnumber(trigdata->tg_relation->rd_att, "id"), &isnull);
        Datum command_text_datum = SPI_getbinval(new_row, trigdata->tg_relation->rd_att, SPI_fnumber(trigdata->tg_relation->rd_att, "command_text"), &isnull);

        int32 id = DatumGetInt32(id_datum);
        char *command_text = TextDatumGetCString(command_text_datum);

        execute_ddl_command(id, command_text);

        pfree(command_text);
    }

    return PointerGetDatum(NULL);
}
