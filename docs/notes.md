# Rough notes

```
arch/x86/kernel/sys_x86_64:89:SYSCALL_DEFINE6(mmap) -> ksys_mmap_pgoff()
mm/mmap.c:1583:ksys_mmap_pgoff() -> vm_mmap_pgoff()
mm/util.c:506:vm_mmap_pgoff() -> do_mmap()
mm/mmap.c:1404:do_mmap()
populate done in:
include/linux/mm.h:2731:mm_populate() -> __mm_populate()
mm/gup.c:1564:__mm_populate()
```
