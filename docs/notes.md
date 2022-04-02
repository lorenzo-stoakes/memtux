# Rough notes

mmap

```
arch/x86/kernel/sys_x86_64:89:SYSCALL_DEFINE6(mmap) -> ksys_mmap_pgoff()
mm/mmap.c:1583:ksys_mmap_pgoff() -> vm_mmap_pgoff()
mm/util.c:506:vm_mmap_pgoff() -> do_mmap()
mm/mmap.c:1404:do_mmap()
populate done in:
include/linux/mm.h:2731:mm_populate() -> __mm_populate()
mm/gup.c:1564:__mm_populate()
mm/gup.c:1457:populate_vma_page_range()
mm/gup.c:1081:__get_user_pages()
mm/gup.c:912:faultin_page()
mm/memory.c:4756:handle_mm_fault()
mm/memory.c:4600:__handle_mm_fault()
mm/memory.c:4496:handle_pte_fault()
mm/memory.c:3707:do_anonymous_page()
mm/memory.c:3798:  \- inc_mm_counter_fast()
```

statm

Reading from procfs:

```
fs/proc/array.c:655:proc_pid_statm()
fs/proc/task_mmu.c:87:task_statm()
include/linux/mm.h:2027:get_mm_counter()
Reads from mm->rss_stat.count
```

Writing to:

One of:
```
mm/memory.c:177:sync_mm_rss()
mm/memory.c:190:add_mm_counter_fast()
mm/memory.c:199:inc_mm_counter_fast()
mm/memory.c:204:check_sync_rss_stat() -> invokes sync_mm_rss()
```

`TASK_RSS_EVENTS_THRESH` determines threshold before update.

We keep stats per-thread in the `task_struct` which on sync get transferred to
the `mm_struct` object.

It's handle_mm_fault():4767 that invokes `check_sync_rss_stat(current)`.
