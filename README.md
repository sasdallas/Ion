# Ion

Ion is a prototype NT-styled kernel.

## Code Style

Ion is my first attempt at trying out newer coding styles, so you may encounter comments
that don't quite fit the rest of the build. As the system becomes more complete, these comments
will gradually filter into a more unified design.

## AI policy

AI can be used for debugging and minor bugfixes, but no code generation.

## Structuring

- `ionkrnl/` contains the core kernel of Ion
- `hal/` contains the HALs of Ion
- `include/` contains the Ion SDK
- `rtl/` contains the run-time library of Ion
- `third_party/` contains misc. vendor-deps

## Kernel structuring notice

Some of the kernel in Ion is not structured the same way as in NT, such as how Ion's
scheduler is located in `Ps` rather than `Ke`. This was an oversight in earlier development,
but will stick around for the forseseable future.

Otherwise, Ion follows a mostly NT-like pattern, with `Ex` containing the executive exposed to userspace (and other parts of the kernel).

## Sources

FreeBSD for the ULE scheduler design.

Windows NT for the overall kernel design.

[The MINTIA2 operating system](https://github.com/xrarch/mintia2) inspired a lot of Ion's scheduler and Mm code

[Boron](https://github.com/iProgramMC/Boron) inspired Ion's core organization principles (as well as some misc NT-related things)
