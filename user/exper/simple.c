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
	struct smaps_entry entry;

	const size_t page_size = getpagesize();
	const size_t page_aligned_bytes = ALIGN_UP(size, page_size);
	const size_t num_pages = page_aligned_bytes / page_size;
	const size_t num_kib = page_aligned_bytes / 1024;

	printf("\n[mapping/unmapping %lu bytes (%lu pages, %lu KiB)]\n\n", size,
	       num_pages, num_kib);

	get_smaps_rollup(&entry);
	print_smaps_entry("before alloc", &entry, NULL);
	struct smaps_entry prev_entry = entry;

	uint *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
			  -1, 0);
	if (addr == MAP_FAILED) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}
	get_smaps_rollup(&entry);
	print_smaps_entry("after alloc", &entry, &prev_entry);
	prev_entry = entry;

	if (munmap(addr, size)) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}
	get_smaps_rollup(&entry);
	print_smaps_entry("after unmap", &entry, &prev_entry);
}

int main(void)
{
	const size_t ns[] = { 10, 1000, 4096, 4097, 10000, 1000000 };

	for (size_t i = 0; i < ARRAY_SIZE(ns); i++) {
		simple_mmap(ns[i]);
	}

	return EXIT_SUCCESS;
}
