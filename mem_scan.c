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

int get_mem_mode(void *curr_addr)
{
	struct sigaction act, oldact;		// signal handler items
	int mem_mode = MEM_NO; 			// assume r/w mode then test
	char read_data, write_data = 1;		// r/w checkers
	char *addr = (char *) curr_addr;	// current address

	// Setup custom SIGSEGV handler
	act.sa_handler = handle_segv;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;		// TODO: NEEDED!!! But why?
	sigaction(SIGSEGV, &act, &oldact);
	
	// Get read/write mode of given address
	// (with seg faults prepped for and handled).
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
		// Reset address value
		*addr = read_data;
	}
		
	// Unset custom SIGSEGV handler
	sigaction(SIGSEGV, &oldact, 0);

	// r/w mode unchanged means no successful read nor write.
	return mem_mode;
}

int get_mem_layout (struct memregion *regions, unsigned int size)
{
	struct memregion curr_region;		// current memregion
	unsigned char curr_mem_mode;		// current r/w mode
	unsigned int r_i = 0;			// current region index
	unsigned int count = 0;			// counts regions
	void *max_addr = (void *) MAX_ADDR;	// max address
	void *curr_addr = (void *) MIN_ADDR;	// current address

	// Setup the first region
	curr_region.from = curr_addr;
	curr_region.mode = (unsigned char) get_mem_mode(curr_addr);
	curr_addr += PAGE_SIZE;

	// Iterate through address space starting after first address.
	while (curr_addr <= max_addr) {
		
		// Get r/w  mode of current address.
		curr_mem_mode = (unsigned char) get_mem_mode(curr_addr);
		
		// Look for new r/w mode and therefore new region.
		if (curr_region.mode != curr_mem_mode) {

			// Set final address of current region.
			if (r_i < size) {
				curr_region.to = curr_addr - PAGE_SIZE;
				regions[r_i] = curr_region;
				count++;
			}

			// Move to next region
			// and set first address and r/w mode of it
			if (++r_i < size) {
				curr_region.from = curr_addr;
				curr_region.mode = curr_mem_mode;
			}
		}

		// See if next increment causes overflow,
		// therefore last possible address.
		if (curr_addr + PAGE_SIZE <= curr_addr) {

			// Set final address of last region.
			if (r_i < size) {
				curr_region.to = curr_addr;
				regions[r_i] = curr_region;
				count++;
			}

			// Exit loop before overflow.
			break;
		}

		// Safe to increment address.
		curr_addr += PAGE_SIZE;
	}

	return count;	// number of found regions.
}

int get_mem_diff (struct memregion *regions, unsigned int howmany,
	struct memregion *thediff, unsigned int diffsize) 
{
	return 0;
}


int main(void)
{

	// Setup our function and tests
	unsigned int r_i, ret_size, print_size, size = MAX_SIZE;
	struct memregion regions[size];
	
	// Run our function
	ret_size = get_mem_layout(regions, size);

	// Get the smaller of size asked for and actual size.
	print_size = size;
	if (ret_size < size) print_size = ret_size;

	// Test output of our function
	for (r_i = 0; r_i < print_size; r_i++) {
		printf("Region %d: %p --> %p, mode: %u\n", 
			r_i,
			regions[r_i].from,
			regions[r_i].to,
			regions[r_i].mode);
		
	}

	return 0;
}

