#define _XOPEN_SOURCE 600
#define _GNU_SOURCE

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "debugs.h"
#include "ybinlogp.h"

void usage(void) {
	fprintf(stderr, "ybinlogp_test [options] binlog\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options\n");
	fprintf(stderr, "  -h       show this help\n");
}

int main(int argc, char** argv) {
	int opt;
	int fd;
	struct ybp_binlog_parser* bp;
	struct ybp_event* evbuf;
	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
			case 'h':
				usage();
				return 0;
			case '?':
				fprintf(stderr, "Unknown argument %c\n", optopt);
				usage();
				return 1;
				break;
		}
	}
	if (optind >= argc) {
		usage();
		return 2;
	}
	if ((fd = open(argv[optind], O_RDONLY|O_LARGEFILE)) <= 0) {
		perror("Error opening file");
		return 1;
	}
	if ((bp = ybp_get_binlog_parser(fd)) == NULL) {
		perror("init_binlog_parser");
		return 1;
	}
	if ((evbuf = malloc(sizeof(struct ybp_event))) == NULL) {
		perror("malloc event");
		return 1;
	}
	ybp_init_event(evbuf);
	while (ybp_next_event(bp, evbuf) >= 0) {
		if (evbuf->type_code == QUERY_EVENT) {
			struct ybp_query_event_safe* s = ybp_event_to_safe_qe(evbuf);
			printf("%s\n", s->statement);
			ybp_dispose_safe_qe(s);
		}
		else if (evbuf->type_code == ROTATE_EVENT) {
			struct ybp_rotate_event_safe* s = ybp_event_to_safe_re(evbuf);
			printf("%s.%llu\n", s->file_name, (long long unsigned)s->next_position);
			ybp_dispose_safe_re(s);
		}
		else if (evbuf->type_code == XID_EVENT) {
			struct ybp_xid_event* s = ybp_event_to_safe_xe(evbuf);
			printf("%llu\n", (long long unsigned)s->id);
			ybp_dispose_safe_xe(s);
		}
		ybp_reset_event(evbuf);
	}
	ybp_dispose_event(evbuf);
	ybp_dispose_binlog_parser(bp);
}
