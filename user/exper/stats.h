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

// Retrieve memory stats for current process and place in `stats`. Returns stats
// struct for convenience.
struct mem_stats *get_mem_stats(struct mem_stats *stats);

// Retrieve rolled up smaps RSS value in pages.
int get_smaps_rss_pages(void);

// Print out statm and smaps_rollup values and determine the delta for each. NOT
// thread-safe.
void print_stats(const char *prefix, bool show_delta);
