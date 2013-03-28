
#include <stdlib.h>
#include <stdio.h>

#include "module.h"


static const char *modules[] = {
	"test",
	NULL
};


struct module *packet_module = NULL;
struct module *log_module = NULL;
struct module *vm_module = NULL;


int main(int argc, char *argv[])
{
	/* Load all registered modules */
	{
		const char **module = modules;
		while (*module) {
			struct module *handle = load_module(*(module++));
			if (handle) {
				switch (handle->type) {
				case PACKET:
					packet_module = handle;
					break;
				case LOG:
					log_module = handle;
					break;
				case VM:
					vm_module = handle;
					break;
				default:
					unload_module(handle);
					break;
				}
			}
		}
	}

	/* Check module status */
	{
		int err = 0;

		if (!packet_module) {
			fprintf(stderr, "error: no packet module found\n");
			err = 1;
		}

		if (!log_module) {
			fprintf(stderr, "error: no log module found\n");
			err = 1;
		}

		if (!vm_module) {
			fprintf(stderr, "error: no vm module found\n");
			err = 1;
		}

		if (err) {
			return 1;
		}
	}

	return 0;
}

