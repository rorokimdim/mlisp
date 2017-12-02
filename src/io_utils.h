#include "mpc.h"

#ifndef MLISP_IO_UTILS_H
#define MLISP_IO_UTILS_H
void print_ast(mpc_ast_t* ast, int level);
char* read_file(char* file_path);
#endif
