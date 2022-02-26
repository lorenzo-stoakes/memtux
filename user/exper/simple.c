#include "macros.h"
#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Simply mmap() and munmap() `size` bytes of memory and output memory stats
// before and after each.
static void simple_mmap(size_t size)
{
	const size_t page_size = getpagesize();

	printf("mapping/unmapping %lu bytes (%lu pages):\n", size,
	       ALIGN_UP(size, page_size) / 4096);

	print_rss_stats("before alloc", false);

	uint *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
			  -1, 0);
	if (addr == MAP_FAILED) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}
	print_rss_stats(" after alloc", true);

	if (munmap(addr, size)) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}
	print_rss_stats(" after unmap", true);
}

int main(void)
{
	const size_t ns[] = { 10, 1000, 4096, 4097, 10000, 1000000 };

	for (size_t i = 0; i < ARRAY_SIZE(ns); i++) {
		simple_mmap(ns[i]);
	}

	return EXIT_SUCCESS;
}
