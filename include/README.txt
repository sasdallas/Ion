This is the Ion SDK header collection.

Ion's SDK includes all headers from every Ion component, including Kernel/Hal/Userspace/Base.

Helpful prefixes to separate:
ion/*.h are files accessible to all components in the project, and maybe global headers if you want those (e.g. rtl.h)
ion/ke/*.h are kernel headers
ion/hal/*.h are HAL headers (note that since the HAL is small ion/hal.h is included)
ion/x86_64/*.h are common architecture-specific functions, different from arch-specific types such as ion/ke/x86_64/*.h