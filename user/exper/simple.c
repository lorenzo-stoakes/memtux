#include <stdbool.h>
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

// Retrieve rolled up smaps RSS value in pages.
static int get_smaps_rss_pages(void)
{
	FILE *fp = fopen("/proc/self/smaps_rollup", "r");

	if (fp == NULL) {
		fprintf(stderr, "Cannot open smaps_rollup file.\n");
		exit(EXIT_FAILURE);
	}

	// Skip first line.
	char dummy[256];
	if (fgets(dummy, sizeof(dummy), fp) == NULL) {
		fprintf(stderr, "Cannot read first line.\n");
		exit(EXIT_FAILURE);
	}

	int ret = 0;
	fscanf(fp, "%s %d", dummy, &ret);

	fclose(fp);

	// Output as kilobytes so convert to pages.
	ret *= 1024;
	ret /= getpagesize();

	return ret;
}

// Print out statm and smaps_rollup values and determine the delta for each. NOT
// thread-safe.
static void print_stats(const char *prefix, bool show_delta)
{
	// Not thread-safe.
	static int prev_rss = -1;
	static int prev_smaps_rss = -1;

	printf("\t%s: ", prefix);

	struct mem_stats stats;
	int rss = get_mem_stats(&stats)->resident;
	printf("rss=%d", rss);
	if (show_delta && prev_rss >= 0)
		printf(" (% 4d), ", rss - prev_rss);
	else
		printf(",        ");

	int smaps_rss = get_smaps_rss_pages();
	printf("smaps_rss=%d", smaps_rss);

	if (show_delta && prev_smaps_rss >= 0)
		printf(" (% 4d)", smaps_rss - prev_smaps_rss);

	printf("\n");

	prev_rss = rss;
	prev_smaps_rss = smaps_rss;
}

// Simply mmap() and munmap() `size` bytes of memory and output changes in data
// page counts.
static void simple_mmap(size_t size)
{
	const size_t page_size = getpagesize();

	printf("mapping/unmapping %lu bytes (%lu pages):\n", size,
	       ALIGN_UP(size, page_size) / 4096);

	print_stats("before alloc", false);
	uint *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
			  -1, 0);
	if (addr == MAP_FAILED) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}
	print_stats(" after alloc", true);

	if (munmap(addr, size)) {
		perror("simple/exper");
		exit(EXIT_FAILURE);
	}
	print_stats(" after unmap", true);
}

int main(void)
{
	const size_t ns[] = { 10, 1000, 4096, 4097, 10000, 1000000 };

	for (size_t i = 0; i < ARRAY_SIZE(ns); i++) {
		simple_mmap(ns[i]);
	}

	return EXIT_SUCCESS;
}
