#ifndef DDL_PARSER_H
#define DDL_PARSER_H

#include "postgres.h"
#include "nodes/nodes.h"
#include "nodes/parsenodes.h"

const char *get_command_type(Node *parsetree);

const char *get_command_tag(Node *parsetree);

bool is_ddl_command(Node *parsetree);

#endif // DDL_PARSER_H