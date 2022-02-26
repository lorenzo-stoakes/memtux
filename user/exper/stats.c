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
	char prefix[256] = "";
	int val = 0;
	char kb[256] = "";

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

// Determine which smaps_entry to target in an smaps_stats object based on
// header line. Mutates `header` string.
static struct smaps_entry *get_target_entry(char *header,
					    struct smaps_stats *stats)
{
	const size_t len = strlen(header);

	// Find first space from right hand side to identify the memory.
	ssize_t i = len - 1;
	for (; i >= 0 && header[i] != ' '; i--)
		;

	// We should never run off the start.
	if (i < 0) {
		fprintf(stderr, "Cannot parse header line: %s\n", header);
		exit(EXIT_FAILURE);
	}

	const char start = header[i + 1];
	if (start == '[') {
		// Mapping defined by its type.

		header[len - 2] = '\0'; // Clear out other ']'.
		const char *type = &header[i + 2]; // Skip " [".

		if (strcmp(type, "stack") == 0)
			return &stats->stack;
		if (strcmp(type, "heap") == 0)
			return &stats->heap;

		return &stats->other;
	} else if (start == '/') {
		// File mapping.
		return &stats->file;
	}

	// Anonymous mapping.
	return &stats->anonymous;
}

struct smaps_stats *get_smaps_stats(struct smaps_stats *stats)
{
	// Reset stats to zero.
	memset(stats, 0, sizeof(*stats));

	FILE *fp = fopen("/proc/self/smaps", "r");

	if (fp == NULL) {
		fprintf(stderr, "Cannot open smaps file.\n");
		exit(EXIT_FAILURE);
	}

	char line[256];
	if (fgets(line, sizeof(line), fp) == NULL) {
		fprintf(stderr, "Cannot read header.\n");
		exit(EXIT_FAILURE);
	}

	struct smaps_entry *entry = get_target_entry(line, stats);
	while (fgets(line, sizeof(line), fp) != NULL) {
		// Header line.
		if (!parse_smaps_field(line, entry))
			entry = get_target_entry(line, stats);
	}

	fclose(fp);

	return stats;
}

void print_smaps_entry(const char *prefix,
		       const struct smaps_entry *entry,
		       const struct smaps_entry *prev_entry)
{
	bool printed_prefix = false;

#define PRINT_FIELD(_field)                                               \
	if (entry->_field > 0 && (prev_entry == NULL ||                   \
				  entry->_field != prev_entry->_field)) { \
		if (!printed_prefix && prefix != NULL) {                  \
			printf("%s:\n", prefix);                          \
			printed_prefix = true;                            \
		}                                                         \
		printf("\t");                                             \
		if (prev_entry != NULL)                                   \
			printf("(%+4d) ",                                 \
			       entry->_field - prev_entry->_field);       \
		printf(#_field "=%d\n", entry->_field);                   \
	}

	PRINT_FIELD(rss);
	PRINT_FIELD(pss);
	PRINT_FIELD(pss_anon);
	PRINT_FIELD(pss_file);
	PRINT_FIELD(pss_shmem);
	PRINT_FIELD(shared_clean);
	PRINT_FIELD(shared_dirty);
	PRINT_FIELD(private_clean);
	PRINT_FIELD(private_dirty);
	PRINT_FIELD(referenced);
	PRINT_FIELD(anonymous);
	PRINT_FIELD(lazy_free);
	PRINT_FIELD(anon_huge_pages);
	PRINT_FIELD(shmem_pmd_mapped);
	PRINT_FIELD(file_pmd_mapped);
	PRINT_FIELD(shared_hugetlb);
	PRINT_FIELD(private_hugetlb);
	PRINT_FIELD(swap);
	PRINT_FIELD(swap_pss);
	PRINT_FIELD(locked);
#undef PRINT_FIELD
}

void print_smaps_stats(const char *prefix,
		       const struct smaps_stats *stats,
		       const struct smaps_stats *prev_stats)
{
	char new_prefix[256];

#define PRINT_ALLOC_TYPE(_type)                                            \
	strncpy(new_prefix, prefix, sizeof(new_prefix));                   \
	strcat(new_prefix, "/" #_type);                                    \
	print_smaps_entry(new_prefix, &stats->_type,                       \
			  prev_stats == NULL ? NULL : &prev_stats->_type);

	PRINT_ALLOC_TYPE(stack);
	PRINT_ALLOC_TYPE(heap);
	PRINT_ALLOC_TYPE(anonymous);
	PRINT_ALLOC_TYPE(file);
	PRINT_ALLOC_TYPE(other);
#undef PRINT_ALLOC_TYPE
}
