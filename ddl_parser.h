#ifndef DDL_PARSER_H
#define DDL_PARSER_H

#include "postgres.h"

const char *get_command_type(Node *parsetree);

const char *get_command_tag(Node *parsetree);

#endif // DDL_PARSER_H
