#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <inttypes.h>		// for standard compliant pointer printing
#include "memlayout.h"
#include "externals.h"

// TODO: REMOVE BEFORE DELIVERY
const unsigned int 
	PAGE_SIZE = 0x10000,
	MIN_SIZE = 2, 
	MAX_SIZE = 65536;

static jmp_buf env;

void handle_segv (int sig_id)
{	
	longjmp(env, 1);
}

unsigned char get_mem_mode_from_access (void *curr_addr)
{
	struct sigaction act, oldact;		// signal handler items
	char read_data, write_data = 1;		// read/write checkers
	char *addr = (char *) curr_addr;	// current address
	unsigned char mem_mode = MEM_NO; 	// assume r/w mode then test

	// Set custom SIGSEGV handler for illegal memory accesses (segfault).
	act.sa_handler = handle_segv;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;		// TODO: NEEDED!!! But why?
	sigaction(SIGSEGV, &act, &oldact);
	
	// Expect for SIGSEGV for certain addresses, skip this if occurs.
	if (!setjmp(env)) {
	
		// Try to read.
		read_data = (char) *addr;
		mem_mode = MEM_RO;
		
		// Try to write.
		*addr = write_data;
		mem_mode = MEM_RW;

		// Reset value of write-accessed address.
		*addr = read_data;

	}

	// Unset custom SIGSEGV handler.
	sigaction(SIGSEGV, &oldact, 0);

	// Return r/w mode. Unchanged means no access.
	return mem_mode;
}

int get_mem_layout (struct memregion *regions, unsigned int size)
{
	struct memregion curr_region;		// current memregion
	unsigned char curr_mem_mode;		// current r/w mode
	unsigned int r_i = 0;			// current region index
	unsigned int count = 1;			// counts regions
	void *curr_addr = (void *) 0x0;		// current address (start low)

	// Setup the first region's intial address and r/w mode.
	curr_region.from = curr_addr;
	curr_region.mode = get_mem_mode_from_access(curr_addr);

	// Iterate through address space until overflow.
	while (curr_addr + PAGE_SIZE > curr_addr) {

		// Get next address and its r/w mode.
		curr_addr += PAGE_SIZE;
		curr_mem_mode = get_mem_mode_from_access(curr_addr);
		
		// Same r/w mode means same region, consider next address.
		if (curr_region.mode == curr_mem_mode) 
			continue;

		// New region found!
		// Get the last address of previous region.
		curr_region.to = curr_addr - PAGE_SIZE;
		 
		// Save region if returnable.
		if (r_i < size) {
			regions[r_i] = curr_region;
			r_i++;
		}

		// Prep next region.
		curr_region.from = curr_addr;
		curr_region.mode = curr_mem_mode;
		count++;
	}

	// Set final address of last region if returnable.
	curr_region.to = curr_addr;
	if (r_i < size) {
		regions[r_i] = curr_region;
	}

	return count;	// number of found regions.
}

unsigned char get_mem_mode_from_layout (struct memregion *regions, 
	unsigned int howmany, char *addr) 
{
	unsigned int r_i;
	unsigned char mem_mode;
	struct memregion curr_region;

	for (r_i = 0; r_i < howmany; r_i++) {
		curr_region = regions[r_i];
		if (addr >= (char *) curr_region.from 
			&& addr <= (char *) curr_region.to) {
			mem_mode = curr_region.mode;
			break;
		}
	}

	return mem_mode;
}

int get_mem_diff (struct memregion *regions, unsigned int howmany,
	struct memregion *thediff, unsigned int diffsize) 
{	
	unsigned int d_i = 0;
	unsigned int count = 0;
	char *curr_addr = (char *) 0x0;
	unsigned char curr_mem_mode, exp_mem_mode;
	struct memregion curr_diff;
	int search_flag = 1, end_flag = 0;

	while (curr_addr < curr_addr + PAGE_SIZE) {
		
		exp_mem_mode = 
			get_mem_mode_from_layout(regions, howmany, curr_addr);
		curr_mem_mode = 
			get_mem_mode_from_access(curr_addr);

		if (curr_mem_mode != exp_mem_mode && search_flag) {
			curr_diff.from = curr_addr;
			curr_diff.mode = curr_mem_mode;
			search_flag = 0;
		}
	
		if (!search_flag) {

			if ((curr_mem_mode == exp_mem_mode)) {
				end_flag = 1;
			}

			if (curr_diff.mode != curr_mem_mode) {
				end_flag = 1;
			}

		}

		if (end_flag) {
			curr_diff.to = curr_addr - PAGE_SIZE;
			count++;
			if (d_i < diffsize) {
				thediff[d_i] = curr_diff;
				d_i++;
			}
			search_flag = 1;
			end_flag = 0;

		}


		curr_addr += PAGE_SIZE;

	}


	// TODO What happens to last address?

	return count;
}

void print_region (struct memregion region)
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

int main (void)
{
	// Setup our function and tests
	unsigned int r_i, howmany,
		regions_size = MAX_SIZE, diff_size = MAX_SIZE;
	int ret;
	struct memregion regions[regions_size], thediff[diff_size];
	
	// Run our function
	ret = get_mem_layout(regions, regions_size);

	// Get the smaller of size asked for and actual size.
	howmany = regions_size;
	if (ret < regions_size)
		howmany = ret;

	printf("---%d regions found.---\n", ret);

	// Test output of our function
	for (r_i = 0; r_i < howmany; r_i++)
		print_region(regions[r_i]);
		
	// Run our second function
	ret = get_mem_diff(regions, howmany, thediff, diff_size);
	
	// Get the smaller of size asked for and actual size.
	howmany = diff_size;
	if (ret < diff_size)
		howmany = ret;

	printf("---%d diffs found.---\n", ret);

	// Test output of our function
	for (r_i = 0; r_i < howmany; r_i++)
		print_region(thediff[r_i]);

	while (1) {}

	return 0;
}

