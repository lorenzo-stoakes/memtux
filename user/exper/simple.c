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

	// Gather initial statistics.
	struct mem_stats mem_stats;
	int prev_rss = get_mem_stats(&mem_stats)->resident;
	struct smaps_stats stats;
	get_smaps_stats(&stats);
	struct smaps_stats prev_stats = stats;
	struct smaps_entry rollup_stats;
	get_smaps_rollup(&rollup_stats);
	struct smaps_entry prev_rollup_stats = rollup_stats;

	uint *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
			  -1, 0);
	if (addr == MAP_FAILED) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}

	// Output post-mapping stats.
	int rss = get_mem_stats(&mem_stats)->resident;
	printf("[statm] after alloc/rss=%d (%+4d)\n",
	       rss, rss - prev_rss);
	prev_rss = rss;
	get_smaps_stats(&stats);
	print_smaps_stats("[smap]  after alloc", &stats, &prev_stats);
	prev_stats = stats;
	get_smaps_rollup(&rollup_stats);
	print_smaps_entry("[smapr] after alloc", &rollup_stats, &prev_rollup_stats);
	prev_rollup_stats = rollup_stats;

	if (munmap(addr, size)) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}

	// Output post-unmapping stats.
	rss = get_mem_stats(&mem_stats)->resident;
	printf("[statm] after unmap/rss=%d (%+4d)\n",
	       rss, rss - prev_rss);
	get_smaps_stats(&stats);
	print_smaps_stats("[smap]  after unmap", &stats, &prev_stats);
	get_smaps_rollup(&rollup_stats);
	print_smaps_entry("[smapr] after unmap", &rollup_stats, &prev_rollup_stats);
	prev_rollup_stats = rollup_stats;
}

int main(void)
{
	const size_t ns[] = { 10, 1000, 4096, 4097, 10000, 1000000 };

	for (size_t i = 0; i < ARRAY_SIZE(ns); i++) {
		simple_mmap(ns[i]);
	}

	return EXIT_SUCCESS;
}
