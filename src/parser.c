#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

parser_t* parser_build() {
    parser_t* parser = malloc(sizeof(parser_t));
    parser->integer = mpc_new("integer");
    parser->real = mpc_new("real");
    parser->boolean = mpc_new("boolean");
    parser->symbol = mpc_new("symbol");
    parser->string = mpc_new("string");
    parser->sexpr = mpc_new("sexpr");
    parser->qexpr = mpc_new("qexpr");
    parser->expr = mpc_new("expr");
    parser->lispy = mpc_new("lispy");

    mpc_err_t* err = mpca_lang_contents(
        MPCA_LANG_DEFAULT,
        "resources/lisp.grammar",
        parser->integer,
        parser->real,
        parser->boolean,
        parser->symbol,
        parser->string,
        parser->sexpr,
        parser->qexpr,
        parser->expr,
        parser->lispy);

    if (err) {
        mpc_err_print(err);
        mpc_err_delete(err);
        return NULL;
    }

    return parser;
}

void parser_cleanup(parser_t* parser) {
    mpc_cleanup(
        9,
        parser->integer,
        parser->real,
        parser->boolean,
        parser->symbol,
        parser->string,
        parser->sexpr,
        parser->qexpr,
        parser->expr,
        parser->lispy);
    free(parser);
}

int parser_parse(parser_t* parser, char* input, mpc_result_t* result_p) {
  return mpc_parse("<stdin>", input, parser->lispy, result_p);
}
