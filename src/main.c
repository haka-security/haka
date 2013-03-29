
#include <stdlib.h>
#include <stdio.h>

#include "module.h"
#include <haka/filter_module.h>
#include <haka/packet_module.h>


static const char *modules[] = {
	"test",
	"lua",
	NULL
};


struct packet_module *packet_module = NULL;
struct module *log_module = NULL;
struct filter_module *filter_module = NULL;


int main(int argc, char *argv[])
{
	/* Load all registered modules */
	{
		const char **module = modules;
		while (*module) {
			struct module *handle = load_module(*(module++));
			if (handle) {
				switch (handle->type) {
				case MODULE_PACKET:
					packet_module = (struct packet_module *)handle;
					break;
				case MODULE_LOG:
					log_module = handle;
					break;
				case MODULE_FILTER:
					filter_module = (struct filter_module *)handle;
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
			fprintf(stderr, "warning: no log module found\n");
		}

		if (!filter_module) {
			fprintf(stderr, "error: no filter module found\n");
			err = 1;
		}

		if (err) {
			return 1;
		}
	}

	/* Main loop */
	{
		void *pkt = NULL;
		while (pkt = packet_module->receive()) {
			filter_result result = filter_module->filter(pkt);
			packet_module->verdict(pkt, result);
		}
	}

	return 0;
}

