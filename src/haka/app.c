
#include <haka/packet_module.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "app.h"


static char *configuration_file;


int set_configuration_script(const char *file)
{
	free(configuration_file);
	configuration_file = NULL;

	if (file)
		configuration_file = strdup(file);

	return 0;
}

int has_configuration_script()
{
	return (configuration_file != NULL);
}

const char *get_configuration_script()
{
	return configuration_file;
}

char directory[1024];

const char *get_app_directory()
{
	return directory;
}
