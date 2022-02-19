# memtux

memtux is intended to be an experimental playground for the linux memory
management subsystem consisting of both experiments with the kernel and userland
code.

## The kernel

I have taken a stock 5.16 kernel and stripped out a number of unnecessary
components in order to reduce the weight of code and to keep the codebase
focused on mm changes. It is intended to be x86-64 only and run under qemu (my
[kernel scripts](https://github.com/lorenzo-stoakes/kernel-scripts) may come in
handy here!). It is located in the `linux/` subdirectory.

To configure the kernel run `scripts/kern-config`, to configure and build the
kernel run `scripts/kern-build`.

## License

As this codebase contains linux kernel source the entire thing is licensed under
[GPLv2](/LICENSE).
