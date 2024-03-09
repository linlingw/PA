#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  //todo
  vaddr_t gate_addr = cpu.idtr.base + sizeof(GateDesc) * NO;
  assert(gate_addr <= cpu.idtr.base + cpu.idtr.limit);

  memcpy(&t1, &cpu.eflags, sizeof(cpu.eflags));
  rtl_li(&t0, t1);
  rtl_push(&t0);
  
  cpu.eflags.IF = 0;//

  rtl_push(&cpu.cs);
  rtl_li(&t0, ret_addr);
  rtl_push(&t0);

  uint32_t high_addr,low_addr;
  low_addr = vaddr_read(gate_addr, 2);
  high_addr = vaddr_read(gate_addr + sizeof(GateDesc) - 2, 2);
  uint32_t target_addr = (high_addr << 16) + low_addr;

  decoding.jmp_eip = target_addr;
  decoding.is_jmp = true;

#ifdef DEBUG
  Log("target_addr = 0x%x", target_addr);
#endif
}

void dev_raise_intr() {
  cpu.INTR = true;
}
