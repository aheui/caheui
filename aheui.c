#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPACE_WIDTH 100
#define SPACE_HEIGHT 100
#define STACK_CAPACITY 100

int space[SPACE_HEIGHT][SPACE_WIDTH];
int width, height;
int current_stack = 0;
int stack[28][STACK_CAPACITY];
int stack_length[28];
int value_table[] = {0, 2, 4, 4, 2, 5, 5, 3, 5, 7, 9, 9, 7, 9, 9, 8, 4, 4, 6, 2, 4, 1, 3, 4, 3, 4, 4, 3};
int limit_step = 0;

void init_stack() {
	int i, j;
	for (i = 0; i < 28; i++) {
		for (j = 0; j < STACK_CAPACITY; j++) {
			stack[i][j] = 0;
		}
		stack_length[i] = 0;
	}
}
void init_space() {
	int i, j;
	for (i = 0; i < SPACE_HEIGHT; i++) {
		for (j = 0; j < SPACE_WIDTH; j++) {
			space[i][j] = 0;
		}
	}
}

int pop_from(int stack_index) {
	stack_length[stack_index]--;
	int value = stack[stack_index][stack_length[stack_index]];
	stack[stack_index][stack_length[stack_index]] = 0;
	return value;
}
int pop() {
	return pop_from(current_stack);
}
void push_to(int stack_index, int value) {
	stack[stack_index][stack_length[stack_index]] = value;
	stack_length[stack_index]++;
}
void push(int value) {
	push_to(current_stack, value);
}

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
			space[y][x] = c;
			x++;
		}
	}
	height = y;
}

int execute() {
	int x = 0, y = 0;
	int dx = 1, dy = 0;
	int a, b;
	int step = 0;

	while (limit_step == 0 || step < limit_step) {
		int c = space[y][x];
		if (c >= 0xac00 && c <= 0xd7a3) {
			int value;
			c -= 0xac00;
			value = c % 28;
			switch (c / 28 % 21) {
				case 0:  dx=1;  dy=0;  break;
				case 2:  dx=2;  dy=0;  break;
				case 4:  dx=-1; dy=0;  break;
				case 6:  dx=-2; dy=0;  break;
				case 8:  dx=0;  dy=-1; break;
				case 12: dx=0;  dy=-2; break;
				case 13: dx=0;  dy=1;  break;
				case 17: dx=0;  dy=2;  break;

				case 18: dy=-dy;         break;
				case 19: dx=-dx; dy=-dy; break;
				case 20: dx=-dx;         break;
			}
			switch (c / 28 / 21) {
				case 2: a = pop(); b = pop(); push(b/a); break;
				case 3: a = pop(); b = pop(); push(b+a); break;
				case 4: a = pop(); b = pop(); push(b*a); break;
				case 5: a = pop(); b = pop(); push(b%a); break;
				case 6:
					if (value == 21) {
						printf("%d", pop());
					} else if (value == 27) {
						print_uchar(pop());
					} else {
						pop();
					}
				break;
				case 7:
					if (value == 21) {
						int n;
						printf("Input number: ");
						scanf("%d", &n);
						push(n);
					} else if (value == 27) {
						printf("Input character: ");
						push(fgetuc(stdin)); // TODO: length >= 2
					} else {
						push(value_table[value]);
					}
				break;
				case 8:
					// TODO: queue
					a = pop();
					push(a); push(a);
				break;
				case 9: current_stack = value; break;
				case 10: push_to(value, pop()); break;
				case 12: a = pop(); b = pop(); push((b>=a) ? 1 : 0); break;
				case 14: if (pop() == 0) { dx=-dx; dy=-dy; } break;
				case 16: a = pop(); b = pop(); push(b-a); break;
				case 17:
					if (value == 21) {
						a = stack[21][0];
						stack[21][0] = stack[21][1];
						stack[21][1] = a;
					} else if (value == 27) {
						// nop; do nothing
					} else {
						a = pop(); b = pop(); push(a); push(b);
					}
				break;
				case 18: return step; break;  
			}
		}
		x += dx;
		y += dy;

		if (y < 0) y = height - 1;
		if (y >= height) y = 0;

		if (x < 0) x = width - 1;
		if (x >= width && dx != 0) x = 0;

		step++;
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

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [-l limit] filename\n", argv[0]);
		return 1;
	}

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-l")) {
			limit_step = atoi(argv[++i]);
		} else {
			path = argv[i];
		}
	}

	init_space();
	fp = fopen(path, "r");
	input(fp);
	fclose(fp);

	init_stack();
	fprintf(stderr, "\n%d instructions were executed.\n", execute());

	return 0;
}