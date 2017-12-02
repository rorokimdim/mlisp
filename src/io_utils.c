#include <stdio.h>

#include "mpc.h"

void print_padding(int size, char* c) {
    for(int i=0; i < size; i++) {
        printf("%s", c);
    }
}

void print_ast(mpc_ast_t* ast, int level) {
    if (ast->tag && ast->tag[0] != '\0') {
        print_padding(level, ">");
        printf("Tag: %s\n", ast->tag);
    }

    if (ast->contents && ast->contents[0] != '\0') {
        print_padding(level, ">");
        printf("Contents: %s\n", ast->contents);
    }

    print_padding(level, ">");
    printf("Number of children: %d\n", ast->children_num);

    for (int i=0; i < ast->children_num; i++) {
        print_ast(ast->children[i], level + 1);
    }
}

char* read_file(char* file_path) {
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        printf("Error reading init file %s\n", file_path);
        return NULL;
    }

    int buffer_size = sizeof(char) * 1000;
    char* buffer = malloc(buffer_size);
    char c;
    int i = 0;
    while ((c = getc(fp)) != EOF) {
        if (i == (buffer_size - 2)) {
            buffer = realloc(buffer, buffer_size * 2);
        }
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    fclose(fp);
    return buffer;
}
