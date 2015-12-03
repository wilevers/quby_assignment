#include <string.h>

#include "return_code.h"

#undef NDEBUG
#include <assert.h>

int main(int argc, char *argv[])
{
	const char *default_string = return_code_string(n_return_codes);

	return_code code;
	for (code = ok; code != n_return_codes; ++code) {
		const char *code_string = return_code_string(code);
		assert(strcmp(code_string, default_string) != 0);		
	}

	return 0;
}
