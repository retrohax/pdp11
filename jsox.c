#include <stdio.h>
#include <sgtty.h>

int strcmp();

#define MAX_KEY 128

#define ST_START      0
#define ST_IN_OBJ     1
#define ST_IN_KEY     2
#define ST_AFTER_KEY  3
#define ST_WANT_VAL   4
#define ST_IN_STR_VAL 5
#define ST_IN_NESTED  6
#define ST_IN_BARE    7

main(argc, argv)
int argc;
char **argv;
{
	char *obj_name;
	int state;
	int c;
	char ch;
	char key_buf[MAX_KEY];
	int key_len;
	int depth;
	int matched;
	int escaped;
	int top_level;
	int is_tty;
	struct sgttyb ottyb, nttyb;

	if (argc < 2) {
		printf("Usage: %s OBJ_NAME\n", argv[0]);
		exit(1);
	}

	obj_name = argv[1];
	state = ST_START;
	key_len = 0;
	depth = 0;
	matched = 0;
	escaped = 0;
	top_level = 0;

	/* Set raw mode if stdin is a tty */
	is_tty = (gtty(0, &ottyb) == 0);
	if (is_tty) {
		nttyb = ottyb;
		nttyb.sg_flags |= RAW;
		nttyb.sg_flags &= ~ECHO;
		stty(0, &nttyb);
	}

	while (read(0, &ch, 1) == 1) {
		c = ch & 0177;
		if (c == '\0' || c == 0177)
			continue;
		switch (state) {
		case ST_START:
			if (c == '{') {
				state = ST_IN_OBJ;
				top_level = 1;
			}
			break;

		case ST_IN_OBJ:
			if (c == '"') {
				state = ST_IN_KEY;
				key_len = 0;
			} else if (c == '}') {
				top_level--;
				if (top_level <= 0)
					state = ST_START;
			}
			break;

		case ST_IN_KEY:
			if (escaped) {
				if (key_len < MAX_KEY - 1)
					key_buf[key_len++] = c;
				escaped = 0;
			} else if (c == '\\') {
				escaped = 1;
			} else if (c == '"') {
				key_buf[key_len] = '\0';
				state = ST_AFTER_KEY;
				matched = (top_level == 1 && strcmp(key_buf, obj_name) == 0);
			} else {
				if (key_len < MAX_KEY - 1)
					key_buf[key_len++] = c;
			}
			break;

		case ST_AFTER_KEY:
			if (c == ':') {
				state = ST_WANT_VAL;
			}
			break;

		case ST_WANT_VAL:
			if (c == '"') {
				state = ST_IN_STR_VAL;
				escaped = 0;
			} else if (c == '{' || c == '[') {
				state = ST_IN_NESTED;
				depth = 1;
				if (matched)
					putchar(c);
			} else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
				state = ST_IN_BARE;
				if (matched)
					putchar(c);
			}
			break;

		case ST_IN_STR_VAL:
			if (escaped) {
				if (matched)
					putchar(c);
				escaped = 0;
			} else if (c == '\\') {
				escaped = 1;
				if (matched)
					putchar(c);
			} else if (c == '"') {
				if (matched) {
					putchar('\n');
					if (is_tty) stty(0, &ottyb);
					exit(0);
				}
				state = ST_IN_OBJ;
			} else {
				if (matched)
					putchar(c);
			}
			break;

		case ST_IN_NESTED:
			if (matched)
				putchar(c);
			if (c == '{' || c == '[') {
				depth++;
			} else if (c == '}' || c == ']') {
				depth--;
				if (depth == 0) {
					if (matched) {
						putchar('\n');
						if (is_tty) stty(0, &ottyb);
						exit(0);
					}
					state = ST_IN_OBJ;
				}
			}
			break;

		case ST_IN_BARE:
			if (c == ',' || c == '}' || c == ']') {
				if (matched) {
					putchar('\n');
					if (is_tty) stty(0, &ottyb);
					exit(0);
				}
				if (c == '}') {
					top_level--;
					if (top_level <= 0)
						state = ST_START;
					else
						state = ST_IN_OBJ;
				} else {
					state = ST_IN_OBJ;
				}
			} else {
				if (matched)
					putchar(c);
			}
			break;
		}
	}

	if (is_tty) stty(0, &ottyb);
	exit(1);
}
