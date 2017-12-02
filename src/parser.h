#ifndef MLISP_PARSER_H
#define MLISP_PARSER_H

#include "mpc.h"

typedef struct {
    mpc_parser_t* integer;
    mpc_parser_t* real;
    mpc_parser_t* boolean;
    mpc_parser_t* symbol;
    mpc_parser_t* string;
    mpc_parser_t* sexpr;
    mpc_parser_t* qexpr;
    mpc_parser_t* expr;
    mpc_parser_t* lispy;
} parser_t;

parser_t* parser_build();
void parser_cleanup(parser_t* parser);
int parser_parse(parser_t* parser, char* input, mpc_result_t* result_p);
#endif
