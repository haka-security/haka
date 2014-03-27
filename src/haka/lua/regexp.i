/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%{
#include <haka/regexp_module.h>

static char *escape_chars(const char *STRING, size_t SIZE) {
	int iter = 0;
	char *str;
	str = malloc(SIZE + 1);
	if (!str) {
		error(L"memory error");
		return NULL;
	}
	while (iter < SIZE) {
		if (STRING[iter] == '%') {
			str[iter] = '\\';
			iter++;
		}
		if (iter < SIZE) {
			str[iter] = STRING[iter];
		}
		iter++;
	}
	str[SIZE] = '\0';
	return str;
}
%}

struct regexp_result {
	int offset;
	int size;

	%extend {
		~regexp_result() {
			free($self);
		}
	}
};

struct regexp_vbresult {
	int offset;
	int size;

	%extend {
		~regexp_vbresult() {
			free($self);
		}
	}
};

struct regexp_sink {
	%extend {
		bool feed(const char *STRING, size_t SIZE, bool eof=false,
		          struct regexp_result **OUTPUT) {
			int ret = $self->regexp->module->feed($self, STRING, SIZE, eof);

			if (ret == REGEXP_MATCH) {
				*OUTPUT = malloc(sizeof(struct regexp_result));
				if (!*OUTPUT)
					error(L"memory error");
				**OUTPUT = $self->result;
			} else {
				*OUTPUT = NULL;
			}

			return (ret == REGEXP_MATCH);
		}

		bool feed(struct vbuffer_sub *vbuf, bool eof=false,
		          struct regexp_result **OUTPUT) {
			int ret = $self->regexp->module->vbfeed($self, vbuf, eof);

			if (ret == REGEXP_MATCH) {
				*OUTPUT = malloc(sizeof(struct regexp_result));
				if (!*OUTPUT)
					error(L"memory error");
				**OUTPUT = $self->result;
			} else {
				*OUTPUT = NULL;
			}

			return (ret == REGEXP_MATCH);
		}

		bool ispartial() {
			return ($self->match == REGEXP_PARTIAL);
		}

		~regexp_sink() {
			$self->regexp->module->free_regexp_sink($self);
		}
	}
};

%newobject regexp::create_sink;
%newobject regexp::match;
struct regexp {
	%extend {
		char *_match(const char *STRING, size_t SIZE) {
			struct regexp_result re_result;
			char *result;

			int ret = $self->module->exec($self, STRING, SIZE, &re_result);

			if (ret != REGEXP_MATCH) return NULL;

			result = malloc(re_result.size+1);
			if (!result) error(L"memory error");

			strncpy(result, STRING+re_result.offset, re_result.size);
			result[re_result.size] = '\0';

			return result;
		}

		struct vbuffer_sub *_match(struct vbuffer_sub *vbuf) {
			struct regexp_vbresult re_result;
			struct vbuffer_sub *result;

			int ret = $self->module->vbexec($self, vbuf, &re_result);

			if (ret != REGEXP_MATCH) return NULL;

			result = malloc(sizeof(struct vbuffer_sub));
			if (!result) error(L"memory error");

			if (!vbuffer_sub_sub(vbuf, re_result.offset, re_result.size, result)) {
				free(result);
				return NULL;
			}

			return result;
		}

		struct regexp_sink *create_sink() {
			return $self->module->create_sink($self);
		}

		~regexp() {
			$self->module->release_regexp($self);
		}
	}
};

%newobject regexp_module::compile;
struct regexp_module {

	%extend {
		char *match(const char *pattern, const char *STRING, size_t SIZE,
			   int options = 0) {
			struct regexp_result re_result;
			char *result;
			char *esc_regexp = escape_chars(pattern, strlen(pattern));
			int ret = $self->match(esc_regexp, options, STRING, SIZE, &re_result);
			free(esc_regexp);

			if (ret != REGEXP_MATCH) return NULL;

			result = malloc(re_result.size+1);
			if (!result) error(L"memory error");

			strncpy(result, STRING+re_result.offset, re_result.size);
			result[re_result.size] = '\0';

			return result;
		}

		struct vbuffer_sub *match(const char *pattern, struct vbuffer_sub *vbuf) {
			struct regexp_vbresult re_result;
			struct vbuffer_sub *result;
			char *esc_regexp = escape_chars(pattern, strlen(pattern));
			int ret = $self->vbmatch(esc_regexp, 0, vbuf, &re_result);
			free(esc_regexp);

			if (ret != REGEXP_MATCH) return NULL;

			result = malloc(sizeof(struct vbuffer_sub));
			if (!result) error(L"memory error");

			if (!vbuffer_sub_sub(vbuf, re_result.offset, re_result.size, result)) {
				free(result);
				return NULL;
			}

			return result;
		}

		struct regexp *compile(const char *pattern, int options = 0) {
			char *esc_regexp = escape_chars(pattern, strlen(pattern));
			struct regexp *ret = $self->compile(esc_regexp, options);
			free(esc_regexp);
			return ret;
		}

		%immutable;
		int CASE_INSENSITIVE { return REGEXP_CASE_INSENSITIVE; }
	}
};

%luacode {
	swig.getclassmetatable('regexp')['.fn'].match = function (self, input)
		meta = getmetatable(input)
		if swig.getclassmetatable('vbuffer_iterator') ~= meta and
			swig.getclassmetatable('vbuffer_iterator_blocking') ~= meta then
			return self:_match(input)
		end

		local sink = self:create_sink()
		local begin = input:copy()
		while true do
			local sub = input:sub('available')
			if not sub then
				return nil
			end
			local match, pos = sink:feed(sub)
			if match then
				begin:advance(pos.offset)
				return begin:sub(pos.size)
			end
		end
	end
}
