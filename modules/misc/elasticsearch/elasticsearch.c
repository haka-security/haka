/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "elasticsearch.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <haka/error.h>
#include <haka/log.h>
#include <haka/thread.h>
#include <haka/container/list2.h>
#include <haka/container/vector.h>

#define MODULE L"elasticsearch"

struct elasticsearch_request {
	struct list2_elem   list;
	enum {
		INSERT,
		UPDATE,
		NEWINDEX,
	}                   request_type;
	char               *index;
	char               *type;
	char               *id;
	char               *data;
};

struct elasticsearch_connector {
	char          *server_address;
	CURL          *curl;
	mutex_t        request_mutex;
	semaphore_t    request_wait;
	struct list2   request;
	struct vector  request_content;
	thread_t       request_thread;
	bool           exit:1;
};


static void free_request(struct elasticsearch_request *req)
{
	free(req->index);
	free(req->type);
	free(req->id);
	free(req->data);
	free(req);
}

static void *elasticsearch_request_thread(void *_connector);


static size_t write_callback_null(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return size*nmemb;
}

struct callback_sting_data {
	char    *string;
	size_t   rem;
};

static size_t read_callback_string(char *buffer, size_t size, size_t nmemb, void *_data)
{
	size_t maxsize;
	struct callback_sting_data *data = _data;

	maxsize = size*nmemb;
	if (maxsize > data->rem) maxsize = data->rem;

	memcpy(buffer, data->string, maxsize);
	data->string += maxsize;
	data->rem -= maxsize;

	return maxsize;
}

static size_t write_callback_string(char *ptr, size_t size, size_t nmemb, void *_data)
{
	struct callback_sting_data *data = _data;

	size *= nmemb;

	data->string = realloc(data->string, data->rem + size + 1);
	if (!data->string) {
		error(L"memory error");
		return 0;
	}

	memcpy(data->string+data->rem, ptr, size);
	*(data->string+data->rem+size) = '\0';
	data->rem += size;

	return size;
}

struct elasticsearch_connector *elasticsearch_connector_new(const char *server)
{
	struct elasticsearch_connector *ret = malloc(sizeof(struct elasticsearch_connector));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	memset(ret, 0, sizeof(struct elasticsearch_connector));

	if (!mutex_init(&ret->request_mutex, false)) {
		free(ret);
		return NULL;
	}

	if (!semaphore_init(&ret->request_wait, 0)) {
		free(ret);
		return NULL;
	}

	list2_init(&ret->request);
	vector_create(&ret->request_content, char, NULL);
	ret->exit = false;

	ret->server_address = strdup(server);
	if (!ret->server_address) {
		elasticsearch_connector_close(ret);
		return NULL;
	}

	ret->curl = curl_easy_init();
	if (!ret->curl) {
		error(L"unable to initialize curl session");
		elasticsearch_connector_close(ret);
		return NULL;
	}

	/* Uses of signal is not possible here in multi-threaded environment */
	curl_easy_setopt(ret->curl, CURLOPT_NOSIGNAL, 1L);

	/* Create request thread */
	if (!thread_create(&ret->request_thread, &elasticsearch_request_thread, ret)) {
		elasticsearch_connector_close(ret);
		return NULL;
	}

	return ret;
}

bool elasticsearch_connector_close(struct elasticsearch_connector *connector)
{
	list2_iter iter, end;

	/* Stop request thread */
	connector->exit = true;
	semaphore_post(&connector->request_wait);
	thread_join(connector->request_thread, NULL);

	end = list2_end(&connector->request);
	for (iter = list2_begin(&connector->request); iter != end; ) {
		struct elasticsearch_request *req = list2_get(iter, struct elasticsearch_request, list);
		iter = list2_erase(iter);
		free_request(req);
	}

	mutex_destroy(&connector->request_mutex);
	semaphore_destroy(&connector->request_wait);
	vector_destroy(&connector->request_content);

	if (connector->curl) curl_easy_cleanup(connector->curl);
	free(connector->server_address);
	free(connector);
	return true;
}

static int elasticsearch_post(struct elasticsearch_connector *connector, const char *url,
		const char *data, json_t **json_res)
{
	CURLcode res;
	struct callback_sting_data reqdata, resdata;
	long ret_code;

	reqdata.string = (char *)data;
	reqdata.rem = strlen(reqdata.string);

	resdata.string = NULL;
	resdata.rem = 0;

	curl_easy_setopt(connector->curl, CURLOPT_POST, 1L);

	if (json_res) {
		curl_easy_setopt(connector->curl, CURLOPT_WRITEFUNCTION, &write_callback_string);
		curl_easy_setopt(connector->curl, CURLOPT_WRITEDATA, &resdata);
	}
	else {
		curl_easy_setopt(connector->curl, CURLOPT_WRITEFUNCTION, &write_callback_null);
		curl_easy_setopt(connector->curl, CURLOPT_WRITEDATA, NULL);
	}

	curl_easy_setopt(connector->curl, CURLOPT_READFUNCTION, &read_callback_string);
	curl_easy_setopt(connector->curl, CURLOPT_READDATA, &reqdata);
	curl_easy_setopt(connector->curl, CURLOPT_POSTFIELDSIZE, reqdata.rem);
	curl_easy_setopt(connector->curl, CURLOPT_POSTFIELDS, NULL);
	curl_easy_setopt(connector->curl, CURLOPT_URL, url);

	curl_easy_setopt(connector->curl, CURLOPT_TIMEOUT, 5);

	res = curl_easy_perform(connector->curl);

	if (res != CURLE_OK) {
		error(L"post error: %s", curl_easy_strerror(res));
		free(resdata.string);
		return -1;
	}

	res = curl_easy_getinfo(connector->curl, CURLINFO_RESPONSE_CODE, &ret_code);
	if (res != CURLE_OK) {
		error(L"post error: %s", curl_easy_strerror(res));
		free(resdata.string);
		return -1;
	}

	messagef(HAKA_LOG_DEBUG, MODULE, L"post successful: %s return %d", url, ret_code);

	/* Check for the rest API return code, treat non 2** has error. */
	if (ret_code < 200 || ret_code >= 300) {
		free(resdata.string);
		return ret_code;
	}

	if (json_res) {
		if (!resdata.string) {
			error(L"post error: invalid json response");
			free(resdata.string);
			return -1;
		}

		*json_res = json_loads(resdata.string, 0, NULL);
		free(resdata.string);

		if (!(*json_res)) {
			error(L"post error: invalid json response");
			return -1;
		}
	}

	return 0;
}

static void append(struct vector *string, const char *str)
{
	const size_t len = strlen(str);
	const size_t index = vector_count(string);

	vector_resize(string, index+len);
	memcpy(vector_get(string, char, index), str, len);
}

static void push_request(struct elasticsearch_connector *connector, struct elasticsearch_request *req)
{
	list2_elem_init(&req->list);

	mutex_lock(&connector->request_mutex);
	list2_insert(list2_end(&connector->request), &req->list);
	semaphore_post(&connector->request_wait);
	mutex_unlock(&connector->request_mutex);
}

#define BUFFER_SIZE    1024

static int do_one_request(struct elasticsearch_connector *connector, const char *url, const char *data)
{
	const int code = elasticsearch_post(connector, url, data, NULL);
	if (check_error()) {
		messagef(HAKA_LOG_ERROR, MODULE, L"request failed: %ls", clear_error());
		return -1;
	}
	return code;
}

static void *elasticsearch_request_thread(void *_connector)
{
	struct elasticsearch_connector *connector = _connector;
	char buffer[BUFFER_SIZE];
	char url[BUFFER_SIZE];

	while (!connector->exit) {
		struct list2 copy;
		list2_iter iter, end;
		int code;

		list2_init(&copy);

		/* Wait for request */
		semaphore_wait(&connector->request_wait);

		/* Get the requests */
		mutex_lock(&connector->request_mutex);
		if (list2_empty(&connector->request)) {
			mutex_unlock(&connector->request_mutex);
			continue;
		}

		list2_swap(&connector->request, &copy);
		mutex_unlock(&connector->request_mutex);

		/* Build request data */
		vector_resize(&connector->request_content, 0);

		end = list2_end(&copy);
		for (iter = list2_begin(&copy); iter != end; ) {
			struct elasticsearch_request *req = list2_get(iter, struct elasticsearch_request, list);

			switch (req->request_type) {
			case NEWINDEX:
				{
					snprintf(url, BUFFER_SIZE, "%s/%s", connector->server_address, req->index);
					code = do_one_request(connector, url, req->data);
					if (code && code != 400) {
						messagef(HAKA_LOG_ERROR, MODULE, L"request failed: %s return error %d", url, code);
					}
				}
				break;

			case INSERT:
			case UPDATE:
				{
					snprintf(buffer, BUFFER_SIZE, "{ \"%s\" : { \"_index\" : \"%s\", \"_type\" : \"%s\"",
							(req->request_type == INSERT ? "index" : "update"), req->index, req->type);
					append(&connector->request_content, buffer);

					if (req->id) {
						snprintf(buffer, BUFFER_SIZE, ", \"_id\" : \"%s\"", req->id);
						append(&connector->request_content, buffer);
					}

					append(&connector->request_content, " } }\n");
					append(&connector->request_content, req->data);
					append(&connector->request_content, "\n");
				}
				break;

			default:
				messagef(HAKA_LOG_ERROR, MODULE, L"invalid request type: %d", req->request_type);
				break;
			}

			iter = list2_erase(iter);
			free_request(req);
		}

		/* Do bulk request if needed :*/
		if (vector_count(&connector->request_content) > 0) {
			snprintf(url, BUFFER_SIZE, "%s/_bulk", connector->server_address);
			*vector_push(&connector->request_content, char) = '\0';

			code = do_one_request(connector, url, vector_first(&connector->request_content, char));
			if (code) {
				messagef(HAKA_LOG_ERROR, MODULE, L"request failed: %s return error %d", url, code);
			}
		}
	}

	return NULL;
}

#define DUPSTR(name) \
	if (name) { \
		req->name = strdup(name); \
		if (!req->name) { \
			error(L"memory error"); \
			free_request(req); \
			return false; \
		} \
	}

static bool elasticsearch_request(struct elasticsearch_connector *connector,
		int reqtype, const char *index, const char *type, const char *id, json_t *data)
{
	struct elasticsearch_request *req;

	assert(connector);

	req = malloc(sizeof(struct elasticsearch_request));
	if (!req) {
		error(L"memory error");
		free(data);
		return false;
	}

	memset(req, 0, sizeof(struct elasticsearch_request));

	req->request_type = reqtype;

	DUPSTR(index);
	DUPSTR(type);
	DUPSTR(id);

	req->data = json_dumps(data, JSON_COMPACT);
	json_decref(data);
	if (!req->data) {
		error(L"cannot dump json object");
		free_request(req);
		return false;
	}

	push_request(connector, req);
	return true;

}

bool elasticsearch_newindex(struct elasticsearch_connector *connector, const char *index, json_t *data)
{
	assert(connector);
	assert(index);

	return elasticsearch_request(connector, NEWINDEX, index, NULL, NULL, data);
}

bool elasticsearch_insert(struct elasticsearch_connector *connector, const char *index,
		const char *type, const char *id, json_t *data)
{

	assert(connector);
	assert(index);
	assert(type);

	return elasticsearch_request(connector, INSERT, index, type, id, data);
}

bool elasticsearch_update(struct elasticsearch_connector *connector, const char *index, const char *type,
		const char *id, json_t *data)
{
	json_t *json_update;

	assert(connector);
	assert(index);
	assert(type);
	assert(id);

	json_update = json_object();
	if (!json_update || json_object_set(json_update, "doc", data)) {
		error(L"memory error");
		json_decref(data);
		return false;
	}

	json_decref(data);

	return elasticsearch_request(connector, UPDATE, index, type, id, json_update);
}

#if 0
/* Elasticsearch synchroneous API not used any more */
bool elasticsearch_insert_sync(struct elasticsearch_connector *connector, const char *index,
		const char *type, const char *id, json_t *doc)
{
	int res;
	size_t len;
	char *url, *json_dump;

	assert(connector);
	assert(index);
	assert(type);

	/* Format uri: <server>/<index>/<type>/id */
	len = strlen(connector->server_address) + strlen(index) + strlen(type) +
			(id ? strlen(id) : 0) + 4;
	url = malloc(len);
	if (!url) {
		error(L"memory error");
		return false;
	}

	if (id) {
		snprintf(url, len, "%s/%s/%s/%s", connector->server_address,
				index, type, id);
	}
	else {
		snprintf(url, len, "%s/%s/%s", connector->server_address,
				index, type);
	}

	json_dump = json_dumps(doc, JSON_COMPACT);
	if (!json_dump) {
		error(L"cannot dump json object");
		free(url);
		return false;
	}

	res = elasticsearch_post(connector, url, json_dump, NULL);
	free(url);
	free(json_dump);
	json_decref(doc);

	if (check_error()) return false;

	if (res < 200 || res >= 300) {
		error(L"post error: result %d %s", res, url);
		return false;
	}

	return true;
}

bool elasticsearch_update_sync(struct elasticsearch_connector *connector, const char *index, const char *type,
		const char *id, json_t *doc)
{
	int res;
	json_t *json_update;
	size_t len;
	char *url, *json_dump;

	assert(connector);
	assert(index);
	assert(type);
	assert(id);

	return false;

	/* Format uri: <server>/<index>/<type>/id/_update */
	len = strlen(connector->server_address) + strlen(index) + strlen(type) + strlen(id) + 12;
	url = malloc(len);
	if (!url) {
		error(L"memory error");
		return false;
	}

	snprintf(url, len, "%s/%s/%s/%s/_update", connector->server_address,
			index, type, id);

	json_update = json_object();
	if (!json_update || json_object_set(json_update, "doc", doc)) {
		error(L"memory error");
		free(url)
		return false;
	}

	json_decref(doc);
	doc = NULL;

	json_dump = json_dumps(json_update, JSON_COMPACT);
	if (!json_dump) {
		error(L"cannot dump json object");
		free(url);
		return false;
	}

	res = elasticsearch_post(connector, url, json_dump, NULL);
	free(url);
	free(json_dump);
	json_decref(doc);

	if (check_error()) return false;

	if (res < 200 || res >= 300) {
		error(L"post error: result %d %s", res);
		return false;
	}

	return true;
}
#endif
