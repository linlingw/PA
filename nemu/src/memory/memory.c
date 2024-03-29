#include "nemu.h"
#include "../../include/device/mmio.h"
#include "memory/mmu.h"

#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xfff)

#define PDX(va)   (((uint32_t)(va) >> 22) & 0x3ff)
#define PTX(va)   (((uint32_t)(va) >> 12) & 0x3ff)
#define OFF(va)   ((uint32_t)(va) & 0xfff)

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int No = is_mmio(addr);
  if(No == -1) 
  {
    return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
  }
  else 
  {
    return mmio_read(addr, len, No);
  }  
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int No=is_mmio(addr);
  if(No == -1) 
  {
    memcpy(guest_to_host(addr), &data, len);
  }
  else 
  {
    mmio_write(addr, len, data, No);
  }
}

paddr_t page_translate(vaddr_t addr,bool iswrite) {
  CR0 t_cr0 = (CR0)cpu.cr0;
  if(t_cr0.paging && t_cr0.protect_enable){
    CR3 t_cr3 = (CR3)cpu.cr3;

    PDE* pgdirs = (PDE*)PTE_ADDR(t_cr3.val);
    PDE pde = (PDE)paddr_read((uint32_t)(pgdirs + PDX(addr)), 4);
    Assert(pde.present,"addr=0x%x",addr);

    PTE* ptab = (PTE*)PTE_ADDR(pde.val);
    PTE pte = (PTE)paddr_read((uint32_t)(ptab + PTX(addr)), 4);
    Assert(pte.present,"addr=0x%x",addr);

    pde.accessed = 1;
    pte.accessed = 1;
    if(iswrite){
      pte.dirty = 1;
    }

    paddr_t paddr = PTE_ADDR(pte.val) | OFF(addr);

    //printf("vaddr = 0x%x,paddr=0x%x\n",addr, paddr);

    return paddr;
  }
  return addr;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if(PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)){
    //printf("error: data cross the page boundary:addr = 0x%x,len=%d!\n",addr,len);
    //assert(0);
    int num1 = 0x1000 - OFF(addr);
    int num2 = len - num1;
    paddr_t paddr1 = page_translate(addr, false);
    paddr_t paddr2 = page_translate(addr + num1, false);

    uint32_t low = paddr_read(paddr1, num1);
    uint32_t high = paddr_read(paddr2, num2);

    uint32_t result = high << (num1 * 8) | low;
    return result;
  }
  else
  {
    paddr_t paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if(PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)){
    //printf("error: data cross the page boundary:addr = 0x%x,len=%d!\n",addr,len);
    //assert(0);
    int num1 = 0x1000 - OFF(addr);
    int num2 = len - num1;
    paddr_t paddr1 = page_translate(addr, true);
    paddr_t paddr2 = page_translate(addr + num1, true);

    uint32_t low = data & (~0u >> ((4 - num1) << 3));
    uint32_t high = data >> ((4 - num2) << 3);

    paddr_write(paddr1,num1,low);
    paddr_write(paddr2,num2,high);
    return;
  }
  else
  {
    paddr_t paddr = page_translate(addr, true);
    paddr_write(paddr, len, data);
  }
}
//to send
