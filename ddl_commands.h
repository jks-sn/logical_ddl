#ifndef DDL_COMMANDS_H
#define DDL_COMMANDS_H

#include "postgres.h"

void insert_ddl_command(const char *command_type, const char *command_tag, const char *command_text);
void execute_ddl_command(int32 id, const char *command_text);

Datum ddl_command_trigger(PG_FUNCTION_ARGS);

#endif // DDL_COMMANDS_H
