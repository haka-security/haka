

struct module {
	void      *handle;

	enum {
		UNKNOWN,
		PACKET,
		LOG,
		VM,
	} type;

	int      (*init)(int argc, char *argv[]);
	void     (*cleanup)();
};

