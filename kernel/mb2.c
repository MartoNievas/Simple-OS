#include <stdint.h>

#include "lib/printf.h"
#include "mb2.h"

#define MB2_TAG_END 0
#define MB2_TAG_MMAP 6

static uint32_t read_u32(uint32_t addr) { return *(uint32_t *)addr; }

static const char *mmap_type_name(uint32_t type) {
  switch (type) {
  case 1:
    return "available";
  case 3:
    return "ACPI reclaimable";
  case 4:
    return "reserved (preserve on hibernate)";
  case 5:
    return "defective";
  default:
    return "reserved";
  }
}

static void parse_mmap_tag(uint32_t tag_addr, uint32_t tag_size) {
  uint32_t entry_size = read_u32(tag_addr + 8);
  uint32_t entries_end = tag_addr + tag_size;

  for (uint32_t entry = tag_addr + 16; entry < entries_end;
       entry += entry_size) {
    uint32_t base_lo = read_u32(entry + 0);
    uint32_t base_hi = read_u32(entry + 4);
    uint32_t len_lo = read_u32(entry + 8);
    uint32_t len_hi = read_u32(entry + 12);
    uint32_t type = read_u32(entry + 16);

    kprintf("  base=0x%x:%x len=0x%x:%x type=%u (%s)\n", base_hi, base_lo,
            len_hi, len_lo, type, mmap_type_name(type));
  }
}

void mb2_parse(uint32_t mb2_info_phys) {
  uint32_t base = KERNEL_VBASE + mb2_info_phys;

  uint32_t total_size = read_u32(base + 0);
  uint32_t end = base + total_size;

  kprintf("mb2: total_size=%u\n", total_size);

  uint32_t tag = base + 8; // skip the fixed total_size/reserved header

  while (tag < end) {
    uint32_t type = read_u32(tag + 0);
    uint32_t size = read_u32(tag + 4);

    if (type == MB2_TAG_END) {
      break;
    }

    if (type == MB2_TAG_MMAP) {
      kprintf("mb2: memory map:\n");
      parse_mmap_tag(tag, size);
    }

    uint32_t size_aligned = (size + 7) & ~7u; // tags are 8-byte aligned
    tag += size_aligned;
  }
}
