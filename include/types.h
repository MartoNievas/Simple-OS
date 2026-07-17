#ifndef MIKERNEL_TYPES_H
#define MIKERNEL_TYPES_H

#include <stdint.h>

/* KERNEL_VBASE itself comes from the Makefile (-DKERNEL_VBASE=0xC0000000,
   mirrored into link.ld via --defsym) so it exists in exactly one place. */
#ifndef KERNEL_VBASE
#error "KERNEL_VBASE must be defined by the build (see Makefile)"
#endif

typedef uint32_t phys_addr_t;
typedef uint32_t virt_addr_t;

#endif
