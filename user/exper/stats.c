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

// Attempt to parse smaps field from `buf` and add them to appropriate field in
// `stats`. If the format is not as expected or unable to parse, returns false,
// otherwise returns true.
static bool parse_smaps_field(const char *buf, struct smaps_entry *stats)
{
	char prefix[256];
	int val;
	char kb[256];

	if (sscanf(buf, "%s %d %s", prefix, &val, kb) == EOF ||
	    strcmp(kb, "kB") != 0)
		return false;

#define GET_VALUE(_key, _field)                    \
	if (strcmp(prefix, #_key ":") == 0) {      \
		stats->_field += val;              \
		return true;                       \
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

	// Unknown field, ignore.
	return true;
}

// Read smaps fields from `fp` stream into `stats`.
static void read_smaps_rollup_fields(FILE *fp, struct smaps_entry *stats)
{
	// Arbitrary sizes, we're into buffer overfow territory here...
	char line[256];

	// Skip top line.
	if (fgets(line, sizeof(line), fp) == NULL) {
		fprintf(stderr, "Cannot read header.\n");
		exit(EXIT_FAILURE);
	}

	// Read each line and extract fields.
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (!parse_smaps_field(line, stats)) {
			fprintf(stderr, "Cannot parse line: %s\n", line);
			exit(EXIT_FAILURE);
		}
	}
}

struct smaps_entry *get_smaps_rollup(struct smaps_entry *stats)
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

	struct smaps_entry smaps_stats;
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
