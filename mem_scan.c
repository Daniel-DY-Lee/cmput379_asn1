//github test

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include "mem_scan.h"

static const unsigned int 
	PAGE_SIZE = 0x400,
	MIN_SIZE = 2, 
	MAX_SIZE = 65537,
	MIN_ADDR = 0x0, 
	MAX_ADDR = 0xFFFFFFFF;

static jmp_buf env;

void handle_segv(int sig_id)
{	
	longjmp(env, 1);
}

char get_mem_mode(void *curr_addr)
{
	struct sigaction act, oldact;
	char mem_mode = MEM_NO; 		// assume r/w mode then test
	char read_data, write_data = '1';
	char *addr = (char *) curr_addr;

	// Setup custom SIGSEGV handler
	act.sa_handler = handle_segv;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;		// TODO: why? block signals
	sigaction(SIGSEGV, &act, &oldact);
	
	// Get read/write mode of given address
	if (!setjmp(env)) {
		if ((read_data = (char) *addr)) {
			// (Successful read)
			mem_mode = MEM_RO;
		}
	}
	if (!setjmp(env)) {
		*addr = write_data;
		// (Successful write)
		mem_mode = MEM_RW;
	}
		
	// Unset custom SIGSEGV handler
	sigaction(SIGSEGV, &oldact, 0);

	// r/w mode unchanged means no successful read nor write.
	return mem_mode;
}

int get_mem_layout (struct memregion *regions, unsigned int size)
{
	unsigned char curr_mem_mode;		// current r/w mode
	int r_i = 0;				// current region index
	void *max_addr = (void *) MAX_ADDR;	// max address
	void *curr_addr = (void *) MIN_ADDR;	// current address

	// Setup the first region
	regions[r_i].from = curr_addr;
	regions[r_i].mode = get_mem_mode(curr_addr);
	curr_addr += PAGE_SIZE;

	// Iterate through address space starting after first address
	while (curr_addr <= max_addr) {
		
		// Get r/w  mode of current address
		curr_mem_mode = get_mem_mode(curr_addr);

		/* TODO: TEST 
		if (!regions[r_i].mode) {
			// (No r/w mode for current region)
			
			// Note current r/w mode
			regions[r_i].mode = curr_mem_mode;

		} else ...
		*/
		
		if (regions[r_i].mode != curr_mem_mode) {
			// (New r/w mode found, therefore new region)

			// Set final address of current region.
			regions[r_i].to = curr_addr - PAGE_SIZE;
			printf("Region found: %p --> %p, %d\n", 
				regions[r_i].from,
				regions[r_i].to,
				regions[r_i].mode);

			// Move to next region (if available)
			// and set first address and r/w mode of it.
			if (++r_i > size) break;
			regions[r_i].from = curr_addr;
			regions[r_i].mode = curr_mem_mode;
		}

		// Ensure increment address without overflow
		if (curr_addr + PAGE_SIZE <= curr_addr) {
			// (Last possible address)

			// Set final address of last region.
			regions[r_i].to = curr_addr;

			// Exit loop before overflow
			break;
		}
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

