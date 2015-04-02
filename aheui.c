#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPACE_WIDTH 100
#define SPACE_HEIGHT 100
#define STACK_DEFAULT_SIZE 100

#define MASK_REQ_ELEMS (1<<1 | 1<<0)
#define FLAG_FETCH_A (1<<0)
#define FLAG_FETCH_B (1<<1)
#define FLAG_FETCH_AB FLAG_FETCH_B
#define FLAG_FETCH_REMOVE (1<<5)
#define FLAG_FETCH_SWAP (1<<6)

#define FLAG_DIR_SET (1<<2)
#define FLAG_DIR_FLIPX (1<<3)
#define FLAG_DIR_FLIPY (1<<4)
#define FLAG_DIR_FLIPXY (FLAG_DIR_FLIPX | FLAG_DIR_FLIPY)

enum op {
    OP_DIV = 2,
    OP_ADD = 3,
    OP_MUL = 4,
    OP_MOD = 5,
//    OP_PRINT = 6,
//    OP_INPUT = 7,
    OP_DUP = 8,
    OP_SWITCH = 9,
    OP_MOVE = 10,
    OP_CMP = 12,
    OP_BRANCH = 14,
    OP_SUB = 16,
    OP_SWAP = 17,
    OP_EXIT = 18,
    
    OP_NOP = -100,
    OP_PRINT_NUM,
    OP_PRINT_CHAR,
    OP_POP,
    OP_INPUT_NUM,
    OP_INPUT_CHAR,
    OP_PUSH,
};

struct opcode {
    int value;
    enum op op;
    int flags;
    struct dir { int dx, dy; } dir;
};
struct opcode space[SPACE_HEIGHT][SPACE_WIDTH];
int width, height;
int current_stack = 0;
int stack_size = STACK_DEFAULT_SIZE;
int *stack[28];
int *stack_top[28];
int *queue_front;
int *current_stack_top;
int value_table[] = {0, 2, 4, 4, 2, 5, 5, 3, 5, 7, 9, 9, 7, 9, 9, 8, 4, 4, 6, 2, 4, 1, 3, 4, 3, 4, 4, 3};
int limit_step = 0;
int quiet_mode = 0;

void switch_to_stack(int stack_index) {
    stack_top[current_stack] = current_stack_top;
    current_stack = stack_index;
    current_stack_top = stack_top[stack_index];
}
void init_stack() {
    int i, j;
    for (i = 0; i < 28; i++) {
        stack[i] = malloc(sizeof(int) * stack_size);
        for (j = 0; j < stack_size; j++) {
            stack[i][j] = 0;
        }
        stack_top[i] = &stack[i][0];
    }
    current_stack_top = stack_top[current_stack];
    queue_front = stack_top[21];
}
void init_space() {
    int i, j;
    struct opcode noop = {.value = 0, .dir = {0, 0}, .op = OP_NOP, .flags = 0};
    for (i = 0; i < SPACE_HEIGHT; i++) {
        for (j = 0; j < SPACE_WIDTH; j++) {
            space[i][j] = noop;
        }
    }
}

#define _push_to(ptr, value) *(++(ptr)) = (value);
#define push_to(stack_index, value) _push_to(stack_top[stack_index], value)
#define push(value) _push_to(current_stack_top, value)
#define stacksize() (current_stack_top - &stack[current_stack][0])
#define queuesize() (current_stack_top - queue_front)

int fgetuc(FILE *fp) {
    int a = fgetc(fp);

    if (a < 0x80) {
        return a;
    } else if ((a & 0xf0) == 0xf0) {
        return ((a & 0x07) << 18) + ((fgetc(fp) & 0x3f) << 12) + ((fgetc(fp) & 0x3f) << 6) + (fgetc(fp) & 0x3f);
    } else if ((a & 0xe0) == 0xe0) {
        return ((a & 0x0f) << 12) + ((fgetc(fp) & 0x3f) << 6) + (fgetc(fp) & 0x3f);
    } else if ((a & 0xc0) == 0xc0) {
        return ((a & 0x1f) << 6) + (fgetc(fp) & 0x3f);
    } else {
        return -1;
    }
}
void print_uchar(int ch) {
    if (ch < 0x80) {
        putchar(ch);
    } else if (ch < 0x0800) {
        putchar(0xc0 | (ch >> 6));
        putchar(0x80 | (ch & 0x3f));
    } else {
        if (ch < 0x10000) {
            putchar(0xe0 | (ch >> 12));
            putchar(0x80 | ((ch >> 6) & 0x3f));
            putchar(0x80 | (ch & 0x3f));
        }
    }
}

void input(FILE *fp) {
    int x = 0, y = 0;
    width = height = 0;

    while (!feof(fp)) {
        int c = fgetuc(fp);
        if (c == '\n' || c == EOF) {
            if (width < x) {
                width = x;
            }
            x = 0;
            y++;
        } else {
            if (c >= 0xac00 && c <= 0xd7a3) {
                int dx, dy, op;
                struct opcode *cell = &space[y][x];
                c -= 0xac00;
                cell->value = c % 28;
                switch (c / 28 % 21) {
                    case 0:  dx=1;  dy=0;  cell->flags |= FLAG_DIR_SET; break;
                    case 2:  dx=2;  dy=0;  cell->flags |= FLAG_DIR_SET; break;
                    case 4:  dx=-1; dy=0;  cell->flags |= FLAG_DIR_SET; break;
                    case 6:  dx=-2; dy=0;  cell->flags |= FLAG_DIR_SET; break;
                    case 8:  dx=0;  dy=-1; cell->flags |= FLAG_DIR_SET; break;
                    case 12: dx=0;  dy=-2; cell->flags |= FLAG_DIR_SET; break;
                    case 13: dx=0;  dy=1;  cell->flags |= FLAG_DIR_SET; break;
                    case 17: dx=0;  dy=2;  cell->flags |= FLAG_DIR_SET; break;
                    
                    case 18: cell->flags |= FLAG_DIR_FLIPY; break;
                    case 19: cell->flags |= FLAG_DIR_FLIPXY; break;
                    case 20: cell->flags |= FLAG_DIR_FLIPX; break;
                }
                cell->dir.dx = dx;
                cell->dir.dy = dy;
                op = c / 28 / 21;
                if (op == 6) {
                    if (cell->value == 21)
                        op = OP_PRINT_NUM;
                    else if (cell->value == 27)
                        op = OP_PRINT_CHAR;
                    else
                        op = OP_POP;
                } else if (op == 7) {
                    if (cell->value == 21)
                        op = OP_INPUT_NUM;
                    else if (cell->value == 27)
                        op = OP_INPUT_CHAR;
                    else {
                        op = OP_PUSH;
                        cell->value = value_table[cell->value];
                    }
                }
                cell->op = op;
                switch (cell->op) {
                    case OP_DIV: case OP_ADD: case OP_MUL: case OP_MOD: case OP_CMP: case OP_SUB:
                        cell->flags |= FLAG_FETCH_AB;
                        cell->flags |= FLAG_FETCH_REMOVE;
                        break;
                    case OP_SWAP:
                    	cell->flags |= FLAG_FETCH_AB;
                    	cell->flags |= FLAG_FETCH_SWAP;
                    	break;
                    case OP_PRINT_NUM: case OP_PRINT_CHAR: case OP_POP: case OP_MOVE: case OP_BRANCH:
                        cell->flags |= FLAG_FETCH_A;
                        cell->flags |= FLAG_FETCH_REMOVE;
                        break;
                    case OP_DUP:
                        cell->flags |= FLAG_FETCH_A;
                        // non-destructive
                        break;
                    default: break;
                }
            }
            x++;
        }
    }
    height = y;
}

int execute(int *exitcode) {
    int x = 0, y = 0;
    struct dir dir = {0, 1};
    int step = 0;

#ifdef DEBUG
    while (limit_step == 0 || step < limit_step) {
#else
    while (1) {
#endif
        int a, b;
        struct opcode *cell = &space[y][x];
        if (cell->flags & FLAG_DIR_SET) {
            dir = cell->dir;
        }
        else {
            // FLAG_DIR_SET and FLAG_DIR_FLIP* are mutually exclusive
            if (cell->flags & FLAG_DIR_FLIPX) {
                dir.dx = -dir.dx;
            }
            if (cell->flags & FLAG_DIR_FLIPY) {
                 dir.dy = -dir.dy;
            }
        }
        if ((cell->flags & MASK_REQ_ELEMS) != 0) {
            if (current_stack != 21) {
                if (stacksize() < (cell->flags & MASK_REQ_ELEMS))
                    goto underflow;
                a = *current_stack_top;
                if (cell->flags & FLAG_FETCH_B) b = *(current_stack_top - 1);
                if (cell->flags & FLAG_FETCH_REMOVE)
                    current_stack_top -= cell->flags & MASK_REQ_ELEMS;
                if (cell->flags & FLAG_FETCH_SWAP) {
                	*current_stack_top = b;
                	*(current_stack_top - 1) = a;
                }
            } else {
                if (queuesize() < (cell->flags & MASK_REQ_ELEMS))
                    goto underflow;
                a = *(queue_front + 1);
                if (cell->flags & FLAG_FETCH_B) b = *(queue_front + 2);
                if (cell->flags & FLAG_FETCH_REMOVE)
                    queue_front += cell->flags & MASK_REQ_ELEMS;
                if (cell->flags & FLAG_FETCH_SWAP) {
                	*(queue_front + 1) = b;
                	*(queue_front + 2) = a;
                }
            }
        }
        // execute
        switch (cell->op) {
            case OP_NOP: break;
            case OP_DIV: push(b/a); break;
            case OP_ADD: push(b+a); break;
            case OP_MUL: push(b*a); break;
            case OP_MOD: push(b%a); break;
            case OP_PRINT_NUM: printf("%d", a); break;
            case OP_PRINT_CHAR: print_uchar(a); break;
            case OP_POP: break;
            case OP_INPUT_NUM:
                if (!quiet_mode)
                    fprintf(stderr, "Input number: ");
                scanf("%d", &a);
                push(a);
            break;
            case OP_INPUT_CHAR:
                if (!quiet_mode)
                    fprintf(stderr, "Input character: ");
                push(fgetuc(stdin)); // TODO: length >= 2
                break;
            case OP_PUSH: push(cell->value); break;
            case OP_DUP:
                if (current_stack != 21) {
                    push(a);
                } else {
                    if (queue_front == &stack[21][0]) // no space before front
                        memmove(queue_front + 1, queue_front, current_stack_top - queue_front);
                    *queue_front = a;
                    queue_front--;
                }
            break;
            case OP_SWITCH: switch_to_stack(cell->value); break;
            case OP_MOVE:
                if (cell->value == current_stack) {
                    push(a);
                } else {
                    push_to(cell->value, a);
                }
            break;
            case OP_CMP: push((b>=a) ? 1 : 0); break;
            case OP_BRANCH: if (a == 0) { dir.dx = -dir.dx; dir.dy = -dir.dy; } break;
            case OP_SUB: push(b-a); break;
            case OP_SWAP: break;
            case OP_EXIT:
                if (current_stack != 21) {
                    if (stacksize() >= 1)
                        *exitcode = *current_stack_top;
                } else {
                    if (queuesize() >= 1)
                        *exitcode = *(queue_front + 1);
                }
                return step;
        }
        goto next;
    underflow:
        dir.dx = -dir.dx;
        dir.dy = -dir.dy;
    next:
        x += dir.dx;
        y += dir.dy;

        if (y < 0) y = height - 1;
        if (y >= height) y = 0;

        if (x < 0) x = width - 1;
        if (x >= width) x = 0;

#ifdef DEBUG
        step++;
#endif
    }
    if (limit_step != 0) {
        fprintf(stderr, "\naborting;");
    }
    return step;
}

int main(int argc, char *argv[]) {
    FILE *fp;
    int i;
    char *path = NULL;
    char *quiet = NULL;
    int exitcode = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-q] [-l limit] [-s stack size] filename\n", argv[0]);
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-l")) {
            limit_step = atoi(argv[++i]);
#ifndef DEBUG
            fprintf(stderr, "Warning: instruction limiting is only supported in debug mode.\n");
#endif
        } else if (!strcmp(argv[i], "-s")) {
            stack_size = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-q")) {
            quiet_mode = 1;
        } else {
            path = argv[i];
        }
    }

    quiet = getenv("AHEUI_QUIET");
    if (quiet != NULL && !strcmp(quiet, "1")) {
        quiet_mode = 1;
    }

    init_space();
    fp = fopen(path, "r");
    input(fp);
    fclose(fp);

    init_stack();
#ifdef DEBUG
    int step = execute(&exitcode);
    fprintf(stderr, "\n%d instructions were executed.\n", step);
#else
    execute(&exitcode);
#endif

    return exitcode;
}
