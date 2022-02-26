#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Read smaps fields from `fp` pointer.
static void read_smaps_rollup_fields(FILE *fp, struct smaps_rollup *stats)
{
	// Arbitrary sizes, we're into buffer overfow territory here...
	char buf[256], kb[256];
	int val;

	// Skip first line.
	if (fgets(buf, sizeof(buf), fp) == NULL) {
		fprintf(stderr, "Cannot read first line.\n");
		exit(EXIT_FAILURE);
	}

	while (fscanf(fp, "%s %d %s", buf, &val, kb) != EOF) {
		// If we don't see 'kB' in this field, we aren't scanning fields
		// any more.
		if (strcmp(kb, "kB") != 0)
			break;

#define GET_VALUE(_key, _field)                    \
		if (strcmp(buf, #_key ":") == 0) { \
			stats->_field = val;       \
			continue;                  \
		}

		GET_VALUE(Rss, rss);
		GET_VALUE(Pss, pss);
		GET_VALUE(Pss_Anon, pss_anon);
		GET_VALUE(Pss_File, pss_file);
		GET_VALUE(Pss_Shmem, pss_shmem);
		GET_VALUE(Shared_Clean, shared_clean);
		GET_VALUE(Shared_Dirty, shared_dirty);
		GET_VALUE(Private_Clean, private_clean);
		GET_VALUE(Private_Dirty, private_dirty);
		GET_VALUE(Referenced, referenced);
		GET_VALUE(Anonymous, anonymous);
		GET_VALUE(LazyFree, lazy_free);
		GET_VALUE(AnonHugePages, anon_huge_pages);
		GET_VALUE(ShmemPmdMapped, shmem_pmd_mapped);
		GET_VALUE(FilePmdMapped, file_pmd_mapped);
		GET_VALUE(Shared_Hugetlb, shared_hugetlb);
		GET_VALUE(Private_Hugetlb, private_hugetlb);
		GET_VALUE(Swap, swap);
		GET_VALUE(SwapPss, swap_pss);
		GET_VALUE(Locked, locked);
#undef GET_VALUE
	}
}

struct smaps_rollup *get_smaps_rollup(struct smaps_rollup *stats)
{
	// Reset stats to zero.
	memset(stats, 0, sizeof(*stats));

	FILE *fp = fopen("/proc/self/smaps_rollup", "r");

	if (fp == NULL) {
		fprintf(stderr, "Cannot open smaps_rollup file.\n");
		exit(EXIT_FAILURE);
	}

	read_smaps_rollup_fields(fp, stats);
	fclose(fp);

	return stats;
}

void print_rss_stats(const char *prefix, bool show_delta)
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

	struct smaps_rollup smaps_stats;
	int smaps_rss = get_smaps_rollup(&smaps_stats)->rss;
	smaps_rss *= 1024;          // -> bytes
	smaps_rss /= getpagesize(); // -> pages
	printf("smaps_rss=%d", smaps_rss);

	if (show_delta && prev_smaps_rss >= 0)
		printf(" (% 4d)", smaps_rss - prev_smaps_rss);

	printf("\n");

	prev_rss = rss;
	prev_smaps_rss = smaps_rss;
}
