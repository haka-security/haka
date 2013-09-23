
#include <haka/parameters.h>
#include <haka/error.h>

#include <iniparser.h>
#include <assert.h>


#define MAX_SECTION_LEN         256
#define MAX_KEY_LEN             256
#define MAX_FULLKEY_LEN         MAX_SECTION_LEN+MAX_KEY_LEN+1

struct parameters {
	dictionary *iniparser_dict;
	char        section[MAX_SECTION_LEN];
	char        key[MAX_FULLKEY_LEN];
};


struct parameters *parameters_open(const char *file)
{
	struct parameters *ret;

	assert(file);

	ret = malloc(sizeof(struct parameters));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->iniparser_dict = iniparser_load(file);
	if (!ret->iniparser_dict) {
		error(L"configuration file parsing error");
		free(ret->iniparser_dict);
		return NULL;
	}

	ret->section[0] = 0;

	return ret;
}

struct parameters *parameters_create()
{
	struct parameters *ret;

	ret = malloc(sizeof(struct parameters));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->iniparser_dict = dictionary_new(0);
	if (!ret->iniparser_dict) {
		error(L"memory error");
		free(ret->iniparser_dict);
		return NULL;
	}

	ret->section[0] = 0;

	return ret;
}

void parameters_free(struct parameters *params)
{
	assert(params);

	iniparser_freedict(params->iniparser_dict);
	free(params);
}

int parameters_open_section(struct parameters *params, const char *section)
{
	if (strlen(section) >= MAX_SECTION_LEN) {
		error(L"Section name is too long");
		return 1;
	}

	strncpy(params->section, section, MAX_SECTION_LEN - 1);
	params->section[MAX_SECTION_LEN - 1] = '\0';
	return 0;
}

int parameters_close_section(struct parameters *params)
{
	if (params->section[0] != 0) {
		params->section[0] = 0;
		return 0;
	}
	else {
		return 1;
	}
}

static bool parameters_check_key(struct parameters *params, const char *key)
{
	if (strlen(key) >= MAX_KEY_LEN) {
		error(L"Key is too long");
		return false;
	}

	if (params->section[0] != 0) {
		snprintf(params->key, MAX_FULLKEY_LEN, "%s:%s", params->section, key);
	}
	else {
		strncpy(params->key, key, MAX_FULLKEY_LEN - 1);
		params->key[MAX_FULLKEY_LEN - 1] = '\0';
	}

	return true;
}

const char *parameters_get_string(struct parameters *params, const char *key, const char *def)
{
	if (params) {
		if (!parameters_check_key(params, key)) return NULL;
		return iniparser_getstring(params->iniparser_dict, params->key, (char *)def);
	}
	else {
		return def;
	}
}

bool parameters_get_boolean(struct parameters *params, const char *key, bool def)
{
	if (params) {
		if (!parameters_check_key(params, key)) return false;
		return iniparser_getboolean(params->iniparser_dict, params->key, def);
	}
	else {
		return def;
	}
}

int parameters_get_integer(struct parameters *params, const char *key, int def)
{
	if (params) {
		if (!parameters_check_key(params, key)) return false;
		return iniparser_getint(params->iniparser_dict, params->key, def);
	}
	else {
		return def;
	}
}

bool parameters_set_string(struct parameters *params, const char *key, const char *value)
{
	if (!parameters_check_key(params, key)) return false;
	return iniparser_set(params->iniparser_dict, params->key, value) != 0;
}

bool parameters_set_boolean(struct parameters *params, const char *key, bool value)
{
	if (!parameters_check_key(params, key)) return false;
	return iniparser_set(params->iniparser_dict, params->key, value ? "true" : "false") != 0;
}

bool parameters_set_integer(struct parameters *params, const char *key, int value)
{
	char buffer[32];

	if (!parameters_check_key(params, key)) return false;

	snprintf(buffer, 32, "%i", value);
	return iniparser_set(params->iniparser_dict, params->key, buffer) != 0;
}
