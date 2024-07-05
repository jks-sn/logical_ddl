#ifndef DDL_COMMANDS_H
#define DDL_COMMANDS_H

#include "postgres.h"

void insert_ddl_command(const char *command_type, const char *command_tag, const char *schema_name, const char *relation_name, const char *query_string);
void execute_ddl_command(int32 id, const char *command_text);
void handle_ddl_command(const char *command_type, const char *command_tag, const char *schema_name, const char *relation_name, const char *query_string);

#endif // DDL_COMMANDS_H
