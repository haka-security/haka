
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include <haka/config.h>
#include <haka/version.h>
#include <haka/types.h>

#include "config.h"


static void usage(FILE *output, const char *program)
{
	fprintf(stdout, "Usage: %s [options] <command>\n", program);
}

static void help(const char *program)
{
	usage(stdout, program);

	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h,--help:          Display this information\n");
	fprintf(stdout, "\t--version:          Display version information\n");

	fprintf(stdout, "\nCommands:\n");
	fprintf(stdout, "\tstatus:             Display haka daemon status\n");
}

static int parse_cmdline(int *argc, char ***argv)
{
	int c;
	int index = 0;

	static struct option long_options[] = {
		{ "version",      no_argument,       0, 'v' },
		{ "help",         no_argument,       0, 'h' },
		{ 0,              0,                 0, 0 }
	};

	while ((c = getopt_long(*argc, *argv, "h", long_options, &index)) != -1) {
		switch (c) {
		case 'h':
			help((*argv)[0]);
			return 0;

		case 'v':
			printf("version %s, arch %s, %s\n", HAKA_VERSION, HAKA_ARCH, HAKA_LUA);
			printf("API version %d\n", HAKA_API_VERSION);
			return 0;

		default:
			usage(stderr, (*argv)[0]);
			return 2;
		}
	}

	if (optind+1 != *argc) {
		usage(stderr, (*argv)[0]);
		return 2;
	}

	*argc -= optind;
	*argv += optind;
	return -1;
}

static void clean_exit()
{
}

static bool ctl_send(int fd, const char *command)
{
	const size_t len = strlen(command);

	if (send(fd, command, len+1, 0) < 0) {
		fprintf(stderr, "cannot write on ctl socket: %s\n", strerror(errno));
		return false;
	}

	return true;
}

static bool ctl_print(int fd)
{
	char buffer[1024];
	int len;

	do {
		len = recv(fd, buffer, 1024, 0);
		if (len < 0) {
			printf("\n");
			fprintf(stderr, "cannot read from ctl socket: %i, %i %s\n", len , errno, strerror(errno));
			return false;
		}

		printf("%.*s", len, buffer);
	} while (len == 1024);

	printf("\n");
	return true;
}

int main(int argc, char *argv[])
{
	int ret;
	int fd;
	struct sockaddr_un addr;
	socklen_t len;

	ret = parse_cmdline(&argc, &argv);
	if (ret >= 0) {
		clean_exit();
		return ret;
	}

	/* Create the socket */
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "cannot create ctl socket: %s\n", strerror(errno));
		return 3;
	}

	bzero((char *)&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, HAKA_CTL_SOCKET_FILE);

	len = strlen(addr.sun_path) + sizeof(addr.sun_family);
	if (connect(fd, (struct sockaddr *)&addr, len)) {
		fprintf(stderr, "cannot connect ctl socket: %s\n", strerror(errno));
		return 3;
	}

	if (strcasecmp(argv[0], "STATUS") == 0) {
		ctl_send(fd, "STATUS");
		ctl_print(fd);
	}
	else {
		fprintf(stderr, "invalid command '%s', see help for the list of available command\n", argv[0]);
		return 1;
	}

	return 0;
}
