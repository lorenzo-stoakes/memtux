#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct mem_stats *get_mem_stats(struct mem_stats *stats)
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

int get_smaps_rss_pages(void)
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

void print_stats(const char *prefix, bool show_delta)
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
