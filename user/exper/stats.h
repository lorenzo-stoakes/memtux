#pragma once

#include <stdbool.h>

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

// Represents either memory statistics obtained from /proc/[pid]/smaps_rollup or
// an entry in /proc/[pid]/smaps.
struct smaps_entry {
	int rss;              // Resident Set Size (RSS) in KiB.
	int pss;              // Proportional Set Size (PSS) in KiB.
	//    smaps_rollup only ->
	int pss_anon;         // PSS anonymous pages in KiB.
	int pss_file;         // PSS file pages in KiB.
	int pss_shmem;        // PSS shared memory pages in KiB.
	// <- smaps_rollup only
	int shared_clean;     // Shared clean (synced to disk) pages in KiB.
	int shared_dirty;     // Shared dirty (modified, not synced to disk) pages in KiB.
	int private_clean;    // Private clean pages in KiB.
	int private_dirty;    // Private dirty pages in KiB.
	int referenced;       // Pages marked 'accessed' in KiB.
	int anonymous;        // Total anonymous pages in KiB.
	int lazy_free;        // Pages marked madvise(MADV_FREE) in KiB.
	int anon_huge_pages;  // Pages backed by transparent hugepages (THP) in KiB.
	int shmem_pmd_mapped; // Shared pages (shmem/tmpfs) backed by huge pages in KiB.
	int file_pmd_mapped;  // Shared _file_ pages backed by THP in KiB.
	int shared_hugetlb;   // Shared pages backed by hugetlbfs in KiB.
	int private_hugetlb;  // Private pages backed by hugetlbfs in KiB.
	int swap;             // Swapped out pages in KiB.
	int swap_pss;         // As above, only made proportional in KiB.
	int locked;           // Is the mapping locked in memory?
};

// Represents smaps statistics aggregated by different type of allocation.
struct smaps_stats {
	struct smaps_entry stack, heap, anonymous, file, other;
};

// Retrieve memory stats for current process and place in `stats`. Returns stats
// struct for convenience.
struct mem_stats *get_mem_stats(struct mem_stats *stats);

// Retrieve rolled up smaps statistics.
struct smaps_entry *get_smaps_rollup(struct smaps_entry *stats);

// Retrieve smaps statistics aggregated by type of allocation.
struct smaps_stats *get_smaps_stats(struct smaps_stats *stats);

// Print out all non-zero fields in smaps entry and optionally show delta from
// previous entry values if `prev_entry` is non-null. It optionally shows
// `prefix` on the line above if non-null.
void print_smaps_entry(const char *prefix,
		       const struct smaps_entry *entry,
		       const struct smaps_entry *prev_entry);

// Print out aggregated smaps statistics by mapping type. Parameters similar to
// print_smaps_entry().
void print_smaps_stats(const char *prefix,
		       const struct smaps_stats *stats,
		       const struct smaps_stats *prev_stats);
