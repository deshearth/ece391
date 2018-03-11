#include "lib.h"
#include "x86_desc.h" 
#include "i8259.h"

#define RESV_IDT_NUM 	32
#define KBD_STATUS_PORT		0x64
#define KBD_DATA_PORT			0x60
#define KBD_OUT_BUF_MASK  1

int kbd_init(uint32_t kbd_irq_num);
