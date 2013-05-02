
#include <haka/packet_module.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "app.h"


static char *filter_script;


int set_filter_script(const char *file)
{
	free(filter_script);
	filter_script = NULL;

	if (file)
		filter_script = strdup(file);

	return 0;
}

int has_filter_script()
{
	return (filter_script != NULL);
}

const char *get_filter_script()
{
	return filter_script;
}

char directory[1024];

const char *get_app_directory()
{
	return directory;
}
