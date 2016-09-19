#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <inttypes.h>
#include <stdint.h>
#include "mem_scan.h"

static const unsigned int 
	PAGE_SIZE = 0x400,
	MIN_SIZE = 2, 
	MAX_SIZE = 65536;

static jmp_buf env;

void handle_segv(int sig_id)
{	
	longjmp(env, 1);
}

unsigned char get_mem_mode(void *curr_addr)
{
	struct sigaction act, oldact;		// signal handler items
	char read_data, write_data = 1;		// read/write checkers
	char *addr = (char *) curr_addr;	// current address
	int mem_mode = MEM_NO; 			// assume r/w mode then test

	// Set custom SIGSEGV handler for illegal memory accesses (segfault).
	act.sa_handler = handle_segv;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;		// TODO: NEEDED!!! But why?
	sigaction(SIGSEGV, &act, &oldact);
	
	// Try to read, jump here and skip upon any segfaults.
	if (!setjmp(env) && (read_data = (char) *addr)) {

		// Read successful.
		mem_mode = MEM_RO;

		// Try to write.
		*addr = write_data;

		// Write successful.
		mem_mode = MEM_RW;

		// Reset value of accessed address.
		*addr = read_data;
	}
		
	// Unset custom SIGSEGV handler.
	sigaction(SIGSEGV, &oldact, 0);

	// Return r/w mode. Unchanged means no access.
	return (unsigned char) mem_mode;
}

int get_mem_layout (struct memregion *regions, unsigned int size)
{
	struct memregion curr_region;		// current memregion
	unsigned char curr_mem_mode;		// current r/w mode
	unsigned int r_i = 0;			// current region index
	unsigned int count = 0;			// counts regions
	void *curr_addr = (void *) 0x0;		// current address

	// Setup the first region's intial address and r/w mode.
	curr_region.from = curr_addr;
	curr_region.mode = get_mem_mode(curr_addr);

	// Iterate through address space until overflow.
	while (curr_addr + PAGE_SIZE > curr_addr) {

		// Get next address and its r/w mode.
		curr_addr += PAGE_SIZE;
		curr_mem_mode = get_mem_mode(curr_addr);
		
		// Same r/w mode means same region, consider next address.
		if (curr_region.mode == curr_mem_mode) 
			continue;

		// Different memory mode? New region found! 
		// Set final address of current region if returnable.
		if (r_i < size) {
			curr_region.to = curr_addr - PAGE_SIZE;
			regions[r_i] = curr_region;
			count++;
		}

		// Setup for next region if returnable and 
		// intitialize first address and r/w mode of it.
		if (++r_i < size) {
			curr_region.from = curr_addr;
			curr_region.mode = curr_mem_mode;
		}
	}

	// Set final address of last region if returnable.
	if (r_i < size) {
		curr_region.to = curr_addr;
		regions[r_i] = curr_region;
		count++;
	}

	return count;	// number of found regions.
}

int get_mem_diff (struct memregion *regions, unsigned int howmany,
	struct memregion *thediff, unsigned int diffsize) 
{
	return 0;
}

void print_region(struct memregion region)
{
	// http://stackoverflow.com/questions/1255099/whats-the-proper-use-of-printf-to-display-pointers-padded-with-0s

	// Display address range.
	printf("0x%08" PRIxPTR "-0x%08" PRIxPTR " ",
		(uintptr_t) region.from,
		(uintptr_t) region.to);

	// Display r/w mode.
	switch (region.mode) {
		case MEM_RO:
			printf("RO\n");
			break;
		case MEM_RW:
			printf("RW\n");
			break;
		case MEM_NO:
			printf("NO\n");
			break;
		default:
			printf("UNKNOWN\n");
	}
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
	if (ret_size < size)
		print_size = ret_size;

	// Test output of our function
	for (r_i = 0; r_i < print_size; r_i++)
		print_region(regions[r_i]);	

	return 0;
}

