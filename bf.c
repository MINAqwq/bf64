#include <libdragon.h>

static char *memory = NULL;
const static int memsize = 16384; /* 16KiB */

static int idx = 0;

_Noreturn void
fatal(const char *msg)
{
	printf("[FATAL] %s\n", msg);
	while(1);
}

void
interpret(const char *code)
{
	char c;

	for(c = *code; c != 0; c = *(++code)){
		switch(c){
			case '+': memory[idx]++; break;
			case '-': memory[idx]--; break;
			case '<': idx = (idx == 0)
							? (memsize - 1)
							: idx - 1; break;
			case '>': idx = (memsize - 1)
							? 0
							: idx + 1; break;
			case '[': break; /* TODO */
			case ']': break; /* TODO */
			case '.': putchar(memory[idx]); break;
			case ',': break; /* TODO */
			default:
				continue;
		}
	}
}

int
main(void)
{
	console_init();
	
	memory = malloc(memsize);
	if(memory == NULL)
		fatal("Memory allocation failed");

	joypad_init();

	console_clear();

	interpret("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++..");
	putchar('\n');

	while(1);

	return 0;
}
