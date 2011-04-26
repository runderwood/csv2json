#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>

#define C2J_BUF_SIZE 32 
#define C2J_MAX_VAL_SIZE 256

#define C2J_ST_SC 1
#define C2J_ST_ES 2
#define C2J_ST_IN 4
#define C2J_ST_VA 8

#define C2J_DELIM ','
#define C2J_ENCAP '"'
#define C2J_ESCAP '\\'
#define C2J_EOL '\n'

typedef struct {
    char buf[C2J_BUF_SIZE];
    char value_buf[C2J_MAX_VAL_SIZE];
    int value_len;
    int buf_len;
    int state;
    int buf_idx;
} c2j_ctxt;

int c2j_st_encap(c2j_ctxt *context) {
    printf("st encap\n");
    context->state = C2J_ST_IN;
    return 0;
}

int c2j_en_encap(c2j_ctxt *context) {
    context->value_buf[context->value_len] = '\0';
    printf("encap'd value: %s\n", context->value_buf);
    context->value_buf[0] = '\0';
    context->value_len = 0;
    context->state = C2J_ST_SC;
    return 0;
}

int c2j_st_val(c2j_ctxt *context) {
    printf("st unencap\n");
    context->value_buf[0] = '\0';
    context->value_len = 0;
    context->state = C2J_ST_VA;
    return 0;
}

int c2j_en_val(c2j_ctxt *context) {
    context->value_buf[context->value_len] = '\0';
    printf("unencap'd val: %s\n", context->value_buf);
    context->value_buf[0] = '\0';
    context->value_len = 0;
    context->state = C2J_ST_SC;
    return 0;
}

int c2j_st_sc(c2j_ctxt *context) {
    printf("st sc\n");
    context->state = C2J_ST_SC;
    return 0;
}

int c2j_en_sc(c2j_ctxt *context) {
    printf("en sc\n");
    return 0;
}

int c2j_st_esc(c2j_ctxt *context) {
    printf("st esc\n");
    context->state = C2J_ST_ES;
    return 0;
}

int c2j_en_esc(c2j_ctxt *context) {
    printf("en esc\n");
    context->state = C2J_ST_IN;
    return 0;
}

int c2j_en_line(c2j_ctxt *context) {
    printf("en line (%d)\n", context->state);
    return 0;
}

int c2j_read(c2j_ctxt *context, int i) {
    context->buf_idx = i;
    char c = context->buf[i];
    int st = context->state;
    switch(c) {
        case C2J_DELIM:
            switch(st) {
                case C2J_ST_VA:
                    c2j_en_val(context);
                    break;
                case C2J_ST_IN:
                    c2j_en_encap(context);
                    break;
                case C2J_ST_SC:
                default:
                    c2j_en_val(context);
                    c2j_st_sc(context);
                    break;
            }
            break;
        case C2J_ENCAP:
            switch(st) {
                case C2J_ST_ES:
                    c2j_en_esc(context);
                    break;
                case C2J_ST_IN:
                    c2j_en_encap(context);
                    break;
                default:
                    c2j_st_encap(context);
                    break;
            }
            break;
        case C2J_ESCAP:
            switch(st) {
                case C2J_ST_IN:
                    c2j_st_esc(context);
                    break;
                default:
                    // do nothing?
                    // or allow escaping of delim?
                    break;
            }
            break;
        case C2J_EOL:
            switch(st) {
                case C2J_ST_ES:
                    c2j_en_esc(context);
                    break;
                case C2J_ST_IN:
                    c2j_en_encap(context);
                    break;
                case C2J_ST_VA:
                    c2j_en_val(context);
                    break;
                case C2J_ST_SC:
                    c2j_en_sc(context);
                    break;
                default:
                    break;
            }
            c2j_en_line(context);
            break;
        case ' ':
        case '\t':
            switch(c) {
                case C2J_ST_VA:
                case C2J_ST_IN:
                    assert(context->value_len < C2J_MAX_VAL_SIZE);
                    context->value_buf[context->value_len++] = c;
                    context->value_buf[context->value_len] = '\0';
                    break;
                case C2J_ST_SC:
                default:
                    printf("whitespace\n");
                    break;
            }
            break;
        default:
            switch(st) {
                case C2J_ST_SC:
                    c2j_st_val(context);
                default:
                    break;
            }
            assert(context->value_len < C2J_MAX_VAL_SIZE);
            context->value_buf[context->value_len++] = c;
            context->value_buf[context->value_len] = '\0';
            break;
    }
    return 0;
}

int main(int argc, char **argv) {
    c2j_ctxt context;
    context.state = C2J_ST_SC;
    int i;
    while(!feof(stdin)) {
        context.buf_len = fread(&(context.buf), sizeof(char), C2J_BUF_SIZE-1, stdin);
        context.buf[context.buf_len] = '\0';
        for(i=0; i<context.buf_len; i++) {
            c2j_read(&context, i);
        }
    }
    return EXIT_SUCCESS;
}
