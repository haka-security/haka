/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/alert_module.h>
#include <haka/log.h>
#include <haka/error.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define FROM		"<haka@alert.com>"
#define BODY_SIZE	2048

typedef enum {
	NONE,
	SSL,
	STARTTLS
} AUTH;

struct email {
	char *body;
	size_t size;
};

struct mail_alerter {
	struct alerter_module module;
	char *server;
	char *username;
	char *password;
	AUTH auth;
	curl_slist *recipients;
};

CURL *curl;
CURLcode res = CURLE_OK;
struct email email_ctx;

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *instream)
{
	struct email *email_ctx = (struct email *)instream;
	const char *data;

	if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
	  return 0;
	}

	data = email_ctx->body;

	if (email_ctx->size) {
	  size_t len = strlen(data);
	  memcpy(ptr, data, len);
	  email_ctx->body += len;
	  email_ctx->size -= len;

	  return len;
	}

	return 0;
}

static int init(struct parameters *args)
{
	curl = curl_easy_init();
	return 0;
}

static void cleanup()
{
	/* Always cleanup */
	curl_easy_cleanup(curl);
}

static bool send_mail(struct mail_alerter *state, uint64 id, const struct time *time, const struct alert *alert, bool update)
{
	/*************************************************
	 * https://curl.haxx.se/libcurl/c/smtp-mail.html *
	 * https://curl.haxx.se/libcurl/c/smtp-ssl.html  *
	 * https://curl.haxx.se/libcurl/c/smtp-tls.html  *
	 *************************************************/
	if (curl) {
		/* SSL/TLS or STARTTLS based connection */
		if (state->auth) {
			/* Set username and password */
			curl_easy_setopt(curl, CURLOPT_USERNAME, state->username);
			curl_easy_setopt(curl, CURLOPT_PASSWORD, state->password);
		}

		char server[128];
		sprintf(server, "smtp%s://%s", state->auth == SSL ? "s" : "", state->server);

		curl_easy_setopt(curl, CURLOPT_URL, server);

		if (state->auth) {
#ifdef SKIP_PEER_VERIFICATION
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
		}

		if (state->auth == STARTTLS) {
			curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		}

		/* Mail sender */
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);

		/* Mail recipients */
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, state->recipients);

		/* Message body */
		char body[BODY_SIZE];
		sprintf(body, "Subject: [Haka] alert: %s%s\r\n",
			update ? "update " : "", alert_tostring(id, time, alert, "", " ", false));

		email_ctx.body = body;
		email_ctx.size = strlen(body);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &email_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		if (getlevel(log) == HAKA_LOG_DEBUG)
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		/* Send the message */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK) {
			haka.log.error("curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));
			return false;
		}
	}
	return true;
}

static bool do_alert(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	return send_mail((struct mail_alerter *)state, id, time, alert, false);
}

static bool do_alert_update(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	return send_mail((struct mail_alerter *)state, id, time, alert, true);
}

struct alerter_module *init_alerter(struct parameters *args)
{
	struct mail_alerter *mail_alerter = malloc(sizeof(struct mail_alerter));
	if (!mail_alerter) {
		error("memory error");
		return NULL;
	}

	mail_alerter->module.alerter.alert = do_alert;
	mail_alerter->module.alerter.update = do_alert_update;

	/* Mandatory fields: SMTP server, credentials and mail recipients */
	const char *server = parameters_get_string(args, "smtp_server", NULL);
	const char *username = parameters_get_string(args, "username", NULL);
	const char *password = parameters_get_string(args, "password", NULL);
	const char *auth = parameters_get_string(args, "authentication", "NULL");
	const char *recipients = parameters_get_string(args, "recipients", NULL);

	if (!server || !recipients) {
		error("missing mandatory field");
		free(mail_alerter);
		return NULL;
	}

	mail_alerter->server = strdup(server);

	mail_alerter->auth = NONE;
	if (username && password) {
		mail_alerter->username = strdup(username);
		mail_alerter->password = strdup(password);
		mail_alerter->auth = SSL; // default
		if (strcmp(auth, "STARTTLS") == 0)
			mail_alerter->auth = STARTTLS;

	        if (!(mail_alerter->username && mail_alerter->password)) {
			error("memory error");
			free(mail_alerter);
			return NULL;
		}
	}

	while ((recipient = strtok(recipients, ",")) != NULL)
		mail_alerter->recipients = curl_slist_append(mail_alerter->recipients,
			recipient);

	if (!mail_alerter->server || !mail_alerter->recipients) {
		error("memory error");
		free(mail_alerter);
		return NULL;
	}

	return &mail_alerter->module;
}

void cleanup_alerter(struct alerter_module *module)
{
	struct mail_alerter *alerter = (struct mail_alerter *)module;

	free(alerter);
}

struct alert_module HAKA_MODULE = {
	module: {
	        type:        MODULE_ALERT,
	        name:        "Mail alert",
	        description: "Alert output to mail server",
	        api_version: HAKA_API_VERSION,
	        init:        init,
	        cleanup:     cleanup
	},
	init_alerter:    init_alerter,
	cleanup_alerter: cleanup_alerter
};
