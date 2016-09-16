//github test

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include "mem_scan.h"

static const unsigned int 
	PAGE_SIZE = 0x400,
	MIN_SIZE = 2, 
	MAX_SIZE = 65536,
	MIN_ADDR = 0x0, 
	MAX_ADDR = 0xFFFFFFFF;

static jmp_buf env;

void handle_segv(int sig_id)
{	
	longjmp(env, 1);
}

int get_mem_mode(char * addr)
{
	struct sigaction act, oldact;
	int mem_mode = MEM_NO; // assume we can't access address
	char read_data, write_data = '1';

	// Setup custom SIGSEGV handler
	act.sa_handler = handle_segv;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
	sigaction(SIGSEGV, &act, &oldact);
	
	// Get read/write mode of given address
	if (!setjmp(env)) {
		read_data = *addr;
		if (read_data) {
			// printf("Read from %p\n", addr);
			mem_mode = MEM_RO;
		}
	}
	if (!setjmp(env)) {
		*addr = write_data;
		// printf("Wrote to %p\n", addr);
		mem_mode = MEM_RW;
	}
		
	// Unset custom SIGSEGV handler
	sigaction(SIGSEGV, &oldact, 0);

	// No SIGSEGV means successful read AND write.
	return mem_mode;
}

/*
void scan_memory(void)
{
	//int mem_mode;

	char *curr_addr = (char *) MIN_ADDR;

	while (curr_addr <= (char *) MAX_ADDR) {
		get_mem_mode(curr_addr);
		if (curr_addr + PAGE_SIZE < curr_addr)
			//overflow detected
			break;
		curr_addr += PAGE_SIZE;
	}

}

*/

int get_mem_layout (struct memregion *regions, unsigned int size)
{
	int mem_mode;
	//unsigned int region;
	char *curr_addr = (char *) MIN_ADDR;

	while (curr_addr <= (char *) MAX_ADDR) {
		mem_mode = get_mem_mode(curr_addr);
		if (mem_mode != MEM_NO)
			printf("Accessed %p\n", curr_addr);
		if (curr_addr + PAGE_SIZE < curr_addr)
			//overflow detected
			break;
		curr_addr += PAGE_SIZE;
	}

	return 0;
}

int get_mem_diff (struct memregion *regions, unsigned int howmany,
	struct memregion *thediff, unsigned int diffsize) 
{
	return 0;
}


int main(void)
{
	unsigned int size = MAX_SIZE;
	struct memregion regions[size];

	get_mem_layout(regions, size);

	// scan_memory();

	return 0;
}

