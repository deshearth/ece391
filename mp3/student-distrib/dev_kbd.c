#include "dev_kbd.h"

static void kbd_intr_handler();
static int setup_idt(void (*handler)(), uint32_t irq_num);

int kbd_init(uint32_t kbd_irq_num) {

	/* set runtime idt entry */
	if(setup_idt(kbd_intr_handler, kbd_irq_num) == -1)
		return -1;

	enable_irq(kbd_irq_num);
	return 0;


} 

static int setup_idt(void (*handler)(), uint32_t irq_num) {

	if (handler == NULL) {
		printf("No handler found!");
		return -1;
	}
	SET_IDT_ENTRY(idt[RESV_IDT_NUM+irq_num], handler);
	idt[RESV_IDT_NUM+irq_num].seg_selector = KERNEL_CS;

	return 0;

}

static void kbd_intr_handler() {
	if (KBD_OUT_BUF_MASK & inb(KBD_STATUS_PORT))
		printf("%c is pressed!\n", inb(KBD_DATA_PORT));
	else 
		printf("Keyboard buffer is empty, nothing to read\n");
}
