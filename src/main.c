#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <editline/readline.h>

#include "eval.h"
#include "io_utils.h"
#include "mpc.h"
#include "parser.h"
#include "utils.h"

typedef enum {REPL_CONTINUE, REPL_EXIT} repl_instr_t;
typedef enum {REPL_VERBOSITY_SILENT, REPL_VERBOSITY_NORMAL} repl_verbosity_t;

int MAX_INIT_FILE_SIZE = 10000;

char* EXIT_INPUTS[] = {"exit", "quit", "(exit)", "(quit)"};

repl_instr_t eval_input(parser_t* parser, lenv* e, char* input, repl_verbosity_t v) {
    add_history(input);

    for (int i=0; i < ARRAY_LENGTH(EXIT_INPUTS); i++) {
        if (strcmp(input, EXIT_INPUTS[i]) == 0) {
            return REPL_EXIT;
        }
    }

    mpc_result_t result;
    int rc = parser_parse(parser, input, &result);
    if (rc) {
        mpc_ast_t* ast = result.output;
        lval* x = eval(e, ast);
        if (v != REPL_VERBOSITY_SILENT) lval_println(e, x, false);
        lval_del(x);
        mpc_ast_delete(ast);
    } else {
        mpc_err_print(result.error);
        mpc_err_delete(result.error);
    }

    return REPL_CONTINUE;
}

bool is_all_spaces(char* s) {
    for (int i=0; i < strlen(s); i++) {
        if (!isspace(s[i])) return false;
    }
    return true;
}

repl_instr_t eval_all_sexpr_in_string(lenv* e, parser_t* parser, char* input, repl_verbosity_t v) {
    int opened = 0;
    int closed = 0;

    char buffer[1000];
    buffer[0] = '\0';
    int bi = 0;
    int rc = REPL_CONTINUE;

    for(int i=0; i < strlen(input); i++) {
        char c = input[i];
        if (isspace(c) && opened == closed) continue;

        if (c == ';') {
            while (c != '\r' && c != '\n') {
                i++;
                c = input[i];
            }
            continue;
        }

        buffer[bi++] = c;

        if (c == '(') opened++;
        else if (c == ')') closed++;

        if (opened && (opened == closed)) {
            buffer[bi] = '\0';
            rc = eval_input(parser, e, buffer, v);
            bi = 0;
            buffer[0] = '\0';
        }
    }
    buffer[bi] = '\0';

    if (strlen(buffer)) {
        rc = eval_input(parser, e, buffer, v);
    }

    return rc;
}


void load_file(lenv* e, parser_t* parser, char* file_path) {
    char* buffer = read_file(file_path);
    eval_all_sexpr_in_string(e, parser, buffer, REPL_VERBOSITY_SILENT);
    free(buffer);
}

void repl(parser_t* parser, lenv* e) {
    while(1) {
        char* input = readline("mlisp> ");
        if (input == NULL) {
            break;
        }
        repl_instr_t rc = eval_all_sexpr_in_string(e, parser, input, REPL_VERBOSITY_NORMAL);
        free(input);
        if (rc == REPL_EXIT) break;
    }
    lenv_del(e);
    puts("\nGoodbye!");
}

int main(int argc, char** argv) {
    parser_t* parser = parser_build();
    if (!parser) {
        printf("Error loading lisp parser.");
        return 1;
    }

    lenv* e = lenv_new();
    lenv_add_builtins(e);
    load_file(e, parser, "resources/init.el");

    if (argc > 1) {
        for (int i=1; i < argc; i++) {
            load_file(e, parser, argv[i]);
        }
        eval_all_sexpr_in_string(e, parser, "(main)", REPL_VERBOSITY_SILENT);
        return 0;
    }

    puts(" MLISP Version 0.0.0.0.1");
    puts(" Press Ctrl + c to Exit\n");
    repl(parser, e);
    parser_cleanup(parser);

    return 0;
}
