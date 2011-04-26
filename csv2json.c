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
#define C2J_ST_OU 16

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
    int row_count;
    GSList *field_list;
} c2j_ctxt;

c2j_ctxt *c2j_new() {
    c2j_ctxt *context = malloc(sizeof(c2j_ctxt));
    context->field_list = g_slist_alloc();
    return context;
}

int c2j_free(c2j_ctxt *context) {
    g_slist_free(context->field_list);
    free(context);
    return 0;
}

int c2j_st_val(c2j_ctxt *context) {
    context->value_buf[0] = '\0';
    context->value_len = 0;
    context->state = C2J_ST_VA;
    return 0;
}

int c2j_en_val(c2j_ctxt *context) {
    context->value_buf[context->value_len] = '\0';
    char *s = g_strndup(context->value_buf, context->value_len);
    context->field_list = g_slist_append(context->field_list, s);
    context->value_buf[0] = '\0';
    context->value_len = 0;
    context->state = C2J_ST_SC;
    return 0;
}

int c2j_st_encap(c2j_ctxt *context) {
    c2j_st_val(context);
    context->state = C2J_ST_IN;
    return 0;
}

int c2j_en_encap(c2j_ctxt *context) {
    c2j_en_val(context);
    context->state = C2J_ST_OU;
    return 0;
}

int c2j_st_sc(c2j_ctxt *context) {
    context->state = C2J_ST_SC;
    return 0;
}

int c2j_en_sc(c2j_ctxt *context) {
    return 0;
}

int c2j_st_esc(c2j_ctxt *context) {
    context->state = C2J_ST_ES;
    return 0;
}

int c2j_en_esc(c2j_ctxt *context) {
    context->state = C2J_ST_IN;
    return 0;
}

int c2j_en_line(c2j_ctxt *context) {
    context->row_count++;
    // do something with field list
    GSList *nxt = context->field_list;
    while((nxt = g_slist_next(nxt)) != NULL) {
        printf("\"");
        char *c = nxt->data;
        while(*c != '\0') {
            if(*c == C2J_ENCAP)
                printf("\\");
            printf("%c", *c);
            c++;
        } 
        printf("\"");
        if(nxt != g_slist_last(context->field_list))
            printf(",");
    }
    printf("\n");
    // empty field list
    g_slist_free(context->field_list);
    context->field_list = g_slist_alloc();
    context->state = C2J_ST_SC;
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
                    assert(context->value_len < C2J_MAX_VAL_SIZE);
                    context->value_buf[context->value_len++] = c;
                    context->value_buf[context->value_len] = '\0';
                    break;
                case C2J_ST_OU:
                    context->state = C2J_ST_SC;
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
                    assert(context->value_len < C2J_MAX_VAL_SIZE);
                    context->value_buf[context->value_len++] = c;
                    context->value_buf[context->value_len] = '\0';
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
        case '\r':
            switch(c) {
                case C2J_ST_VA:
                case C2J_ST_IN:
                    assert(context->value_len < C2J_MAX_VAL_SIZE);
                    context->value_buf[context->value_len++] = c;
                    context->value_buf[context->value_len] = '\0';
                    break;
                case C2J_ST_SC:
                default:
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
    c2j_ctxt *context = c2j_new();
    context->state = C2J_ST_SC;
    int i;
    while(!feof(stdin)) {
        context->buf_len = fread(&(context->buf), sizeof(char), C2J_BUF_SIZE-1, stdin);
        context->buf[context->buf_len] = '\0';
        for(i=0; i<context->buf_len; i++) {
            c2j_read(context, i);
        }
    }
    printf("read %d rows\n", context->row_count);
    c2j_free(context);
    return EXIT_SUCCESS;
}
