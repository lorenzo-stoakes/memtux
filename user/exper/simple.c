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
	const size_t page_aligned_bytes = ALIGN_UP(size, page_size);
	const size_t num_pages = page_aligned_bytes / page_size;
	const size_t num_kib = page_aligned_bytes / 1024;

	printf("\n[mapping/unmapping %lu bytes (%lu pages, %lu KiB)]\n\n", size,
	       num_pages, num_kib);

	struct smaps_stats stats;
	get_smaps_stats(&stats);
	print_smaps_stats("before alloc", &stats, NULL);
	struct smaps_stats prev_stats = stats;

	uint *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
			  -1, 0);
	if (addr == MAP_FAILED) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}
	get_smaps_stats(&stats);
	print_smaps_stats("after alloc", &stats, &prev_stats);
	prev_stats = stats;

	if (munmap(addr, size)) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}
	get_smaps_stats(&stats);
	print_smaps_stats("after unmap", &stats, &prev_stats);
}

int main(void)
{
	const size_t ns[] = { 10, 1000, 4096, 4097, 10000, 1000000 };

	for (size_t i = 0; i < ARRAY_SIZE(ns); i++) {
		simple_mmap(ns[i]);
	}

	return EXIT_SUCCESS;
}
