#include <stdio.h>
#include <sgtty.h>

#define MAX_STRLEN 80

int readchar(fd, c)
int fd;
char *c;
{
	int n = read(fd, c, 1);
	if (n < 0) {
		printf("Read error\n");
		return 0;
	} else if (n == 0) {
		printf("EOF\n");
		return 0;
	}
	return 1;
}

int utf8char(fd, c)
int fd;
char *c;
{
	char c2;

	if ((*c & 0x80) == 0) {
		return 1;
	} else if ((*c & 0xE0) == 0xC0) {
		if (readchar(fd, &c2) == 0) return 0;
	} else if ((*c & 0xF0) == 0xE0) {
		if (readchar(fd, &c2) == 0) return 0;
		if (readchar(fd, &c2) == 0) return 0;
	} else if ((*c & 0xF8) == 0xF0) {
		if (readchar(fd, &c2) == 0) return 0;
		if (readchar(fd, &c2) == 0) return 0;
		if (readchar(fd, &c2) == 0) return 0;
	}

	*c = '?';
	return 1;
}

ser_send(fd, str)
int fd;
char *str;
{
	write(fd, str, strlen(str));
}

ser_recv(fd, wait_str)
int fd;
char *wait_str;
{
	int k;
	int i = 0;
	char buf[MAX_STRLEN];
	int ws_len = strlen(wait_str);

	if (ws_len >= MAX_STRLEN) {
		printf("Wait string too long\n");
		return;
	}

	for (k = 0; k < MAX_STRLEN; k++) {
		buf[k] = ' ';
	}

	while (1) {
		char c;
		int j;
		int match = 1;
		if (readchar(fd, &c) == 0) break;
		if (utf8char(fd, &c) == 0) break;
		if (c >= ' ' || c == '\r' || c == '\n') {
			putchar(c);
			buf[i] = c;
			i = (i + 1) % MAX_STRLEN;
			for (j = 0; j < ws_len; j++) {
				if (buf[(i - ws_len + j + MAX_STRLEN) % MAX_STRLEN] != wait_str[j]) {
					match = 0;
					break;
				}
			}
			if (match) break;
		}
	}
}

get_req(buf, size)
char *buf;
int size;
{
	int off = 0;
	int n;
	while ((n = fread(buf + off, 1, size - 1 - off, stdin)) > 0) {
		off += n;
		if (off >= size - 1)
			break;
	}
	buf[off] = '\0';
}

main(argc, argv)
int argc;
char **argv;
{
	int fd;
	struct sgttyb ttyb;
	char open_str[256];
	char req_str[1024];
	char *api_host;
	int api_port;
	int use_tls = 0;

	if (argc < 3) {
		printf("Usage: %s host port [use_tls]\n", argv[0]);
		exit(1);
	}

	api_host = argv[1];

	if (sscanf(argv[2], "%d", &api_port) != 1) {
		printf("invalid port\n");
		exit(1);
	}

	if (argc >= 4) {
		if (strcmp(argv[3], "use_tls") == 0) {
			use_tls = 1;
		}
	}

	fd = open("/dev/tty01", 2);
	if (fd < 0) {
		printf("open failed\n");
		exit(1);
	}
	
	/* Configure serial port: 9600 baud, raw interface, no echo */
	ioctl(fd, TIOCGETP, &ttyb);
	ttyb.sg_ispeed = B9600;
	ttyb.sg_ospeed = B9600;
	ttyb.sg_flags |= RAW;
	ttyb.sg_flags &= ~ECHO;
	ioctl(fd, TIOCSETP, &ttyb);
	
	/* Get request from stdin */
	get_req(req_str, sizeof(req_str));

	/* Configure OPEN command */
	if (use_tls) {
		sprintf(open_str, "open %s %d use_tls\r", api_host, api_port);
	} else {
		sprintf(open_str, "open %s %d\r", api_host, api_port);
	}
	
	/* Open connection */
	ser_send(fd, open_str);
	ser_recv(fd, "Escape character is '^]'.\r\n");

	/* Send request */
	ser_send(fd, req_str);
	ser_recv(fd, "hiterm> ");

	printf("\n");

	close(fd);
}
