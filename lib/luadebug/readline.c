
#include "readline.h"
#include "utils.h"

#include <haka/types.h>
#include <editline/readline.h>


static bool readline_initialized = false;

static int empty_generator(const char *text, int state)
{
	return 0;
}

void init_readline(const char *name, CPPFunction* complete)
{
	if (!readline_initialized) {
		rl_initialize();
		readline_initialized = true;
	}

	rl_basic_word_break_characters = " \t\n`@$><=;|&{(";
	rl_readline_name = (char*)name;
	rl_attempted_completion_function = complete;
	rl_completion_entry_function = empty_generator;
	using_history();
}
