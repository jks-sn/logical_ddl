#include "postgres.h"
#include "executor/spi.h"
#include "commands/trigger.h"
#include "utils/builtins.h"
#include "ddl_commands.h"

PG_FUNCTION_INFO_V1(ddl_command_trigger);

void insert_ddl_command(Oid classid, Oid objid, int32 objsubid, const char *commandTag,
                               const char *objectType, const char *schemaName,
                               const char *objectIdentity, const char *commandText) {
    char query[2048];
    SPI_connect();
    snprintf(query, sizeof(query),
             "INSERT INTO logical_ddl.ddl_commands (classid, objid, objsubid, command_tag, object_type, schema_name, object_identity, command_text) "
             "VALUES (%u, %u, %d, '%s', '%s', '%s', '%s', '%s')",
             classid, objid, objsubid, commandTag, objectType, schemaName ? schemaName : "", objectIdentity, commandText);
    SPI_execute(query, false, 0);
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
