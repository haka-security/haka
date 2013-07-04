
#include "readline.h"
#include "utils.h"

#include <readline/readline.h>
#include <readline/history.h>


void rl_display_matches(char **matches, int num_matches, int max_length)
{
	printf(BOLD);
	rl_display_match_list(matches, num_matches, max_length);
	printf(CLEAR);
	rl_on_new_line();
}
