#define MEM_RW 0
#define MEM_RO 1
#define MEM_NO 2

struct memregion {
	void *from;
	void *to;
	unsigned char mode; /* MEM_RW, or MEM_RO, or MEM_NO */
};

int get_mem_layout (struct memregion *regions, unsigned int size);

int get_mem_diff (struct memregion *regions, unsigned int howmany,
	struct memregion *thediff, unsigned int diffsize);
