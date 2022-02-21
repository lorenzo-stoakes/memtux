#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Align `_n` to power-of-2 `_to`.
#define ALIGN(_n, _to) \
	((_n) & ~((_to) - 1))

// Align `_n` to power-of-2 `_to`, either doing nothing if `_n` is already
// aligned or aligning to _next_ multiple of `_to` otherwise.
#define ALIGN_UP(_n, _to) \
	ALIGN((_n) + (_to) - 1, (_to))

// Determine number of elements in array `_arr`.
#define ARRAY_SIZE(_arr) \
	(sizeof(_arr) / sizeof((_arr)[0]))

// Represents process memory statistics obtained from /proc/[pid]/statm.
struct mem_stats {
	int size;     // Program size in pages.
	int resident; // Number of resident pages.
	int shared;   // Number of shared pages.
	int trs;      // Number of 'code' pages.
	int lrs;      // Number of 'library' pages.
	int drs;      // Number of 'data/stack' pages.
	int dt;       // Number of dirty pages.
};

// Retrieve memory stats for current process and place in `stats`. Returns stats
// struct for convenience.
static struct mem_stats *get_mem_stats(struct mem_stats *stats)
{
	FILE *fp = fopen("/proc/self/statm", "r");

	if (fp == NULL) {
		fprintf(stderr, "Cannot open statm file.\n");
		exit(EXIT_FAILURE);
	}

	fscanf(fp, "%d %d %d %d %d %d %d",
	       &stats->size, &stats->resident, &stats->shared, &stats->trs,
	       &stats->lrs, &stats->drs, &stats->dt);

	fclose(fp);

	return stats;
}

// Simply mmap() and munmap() `size` bytes of memory and output changes in data
// page counts.
static void simple_mmap(size_t size)
{
	const size_t page_size = getpagesize();

	printf("mapping/unmapping %lu bytes (%lu pages)\n", size,
	       ALIGN_UP(size, page_size) / 4096);

	struct mem_stats stats;
	int drs = get_mem_stats(&stats)->drs;

	uint *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
			  -1, 0);

	if (addr == MAP_FAILED) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}

	int prev_drs = drs;
	drs = get_mem_stats(&stats)->drs;
	printf("after   mmap drs += %d, ", drs - prev_drs);

	if (munmap(addr, size)) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}

	prev_drs = drs;
	drs = get_mem_stats(&stats)->drs;
	printf("after munmap drs -= %d\n", prev_drs - drs);
}

int main(void)
{
	const size_t ns[] = { 10, 1000, 4096, 4097, 10000, 1000000 };

	for (size_t i = 0; i < ARRAY_SIZE(ns); i++) {
		simple_mmap(ns[i]);
	}

	return EXIT_SUCCESS;
}
