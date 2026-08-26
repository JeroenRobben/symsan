// A multi-byte load spanning a CONCRETE prefix and a SYMBOLIC byte must carry the
// concrete bytes' value into the reconstructed Concat.  __taint_union_load records the
// concrete HIGH byte (as op2) but dropped a concrete LOW byte, leaving op1 = 0, so the
// rebuilt expression read the missing byte as zero.
//
// The two cases below are mirrors of each other and differ only in which end is concrete,
// so case B doubles as the control: it was always correct.
//
// RUN: %ko-clang -o %t %s
// RUN: %t | FileCheck %s
#include <stdint.h>
#include <stdio.h>

typedef uint32_t dfsan_label;
typedef struct {
  uint32_t l1, l2;
  uint64_t op1, op2;
  uint16_t op, size;
  uint32_t hash;
} __attribute__((aligned(8), packed)) dfsan_label_info;

extern dfsan_label dfsan_create_label(uint64_t input_id, uint64_t offset,
                                      uint32_t size_in_bytes);
extern void dfsan_set_label(dfsan_label, void *, unsigned long);
extern dfsan_label dfsan_read_label(const void *, unsigned long);
extern dfsan_label_info *dfsan_get_label_info(dfsan_label);

int main(void) {
  // Case A: low byte concrete (0x03), high byte symbolic.
  static uint8_t a[2] __attribute__((aligned(2)));
  a[0] = 0x03;
  a[1] = 0x04;
  dfsan_set_label(dfsan_create_label(0, 100, 1), &a[1], 1);
  dfsan_label_info *ia = dfsan_get_label_info(dfsan_read_label(a, 2));
  // The concrete low byte 0x03 belongs in op1.  Was 0 before the fix.
  // CHECK: caseA concrete_low=3
  printf("caseA concrete_low=%lu\n", (unsigned long)ia->op1);

  // Case B (control, always worked): low byte symbolic, high byte concrete (0x05).
  static uint8_t b[2] __attribute__((aligned(2)));
  b[0] = 0x07;
  b[1] = 0x05;
  dfsan_set_label(dfsan_create_label(0, 200, 1), &b[0], 1);
  dfsan_label_info *ib = dfsan_get_label_info(dfsan_read_label(b, 2));
  // CHECK: caseB concrete_high=5
  printf("caseB concrete_high=%lu\n", (unsigned long)ib->op2);
  return 0;
}
