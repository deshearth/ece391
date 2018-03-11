#include "types.h"

#define NUM_ENTRY 1024
#define PAGE_SIZE 4096
#define BLANK 0x00000002
#define PAGE_SHIFT 12
#define TAB_SHIFT 22
#define VMEM	0xB8000
#define RW_P_MASK	3
#define PAGE_MASK	0x80000000
uint32_t page_directory[NUM_ENTRY] __attribute__((aligned(PAGE_SIZE)));
uint32_t page_table[NUM_ENTRY] __attribute__((aligned(PAGE_SIZE)));


void paging_setup() {
	uint32_t i;
	for (i = 0; i < NUM_ENTRY; i++) {
		page_table[i] = BLANK;
		page_directory[i] = BLANK;
	}
	page_table[VMEM >> PAGE_SHIFT] = VMEM | RW_P_MASK;
	page_directory[(unsigned int)page_table >> TAB_SHIFT] = (unsigned int)page_table | RW_P_MASK;

	asm volatile ("									\n\
			movl %1,%%cr3								\n\
			movl %%cr0,%%eax						\n\
			orl	 $PAGE_MASK,%%eax				\n\
			"
			: /* no output */
			: "r"((uint32_t)page_directory)
			: "%eax"	/* clobbered reg */
			);


}
