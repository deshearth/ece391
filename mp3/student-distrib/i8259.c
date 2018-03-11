/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {

	uint32_t flags;

	cli_and_save(flags);

	master_mask = 0xff;
	slave_mask = 0xff;

	outb(master_mask, MASTER_8259_MASK_PORT);	/* mask all of 8259A-1 */
	outb(slave_mask, SLAVE_8259_MASK_PORT);	/* mask all of 8259A-2 */

	outb(ICW1, MASTER_8259_PORT);	/* ICW1: select 8259A-1 init */
	outb(ICW2_MASTER, MASTER_8259_PORT);	/* ICW2: high bit of vector # */
	outb(ICW3_MASTER, MASTER_8259_PORT);	/* ICW3: bit vector of slave */
	outb(ICW4, MASTER_8259_PORT);	/* ICW4: ISA=x86 */

	outb(ICW1, SLAVE_8259_PORT);	/* ICW1: select 8259A-2 init */
	outb(ICW2_SLAVE, SLAVE_8259_PORT);	/* ICW2: high bit of vector # */
	outb(ICW3_SLAVE, SLAVE_8259_PORT);	/* ICW3: bit vector of slave */
	outb(ICW4, SLAVE_8259_PORT);	/* ICW4: ISA=x86 */

	restore_flags(flags);

}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
	
	uint8_t mask = ~(1 << (irq_num & 7));
	uint32_t flags;

	cli_and_save(flags);
	if (irq_num & 8) {
		slave_mask &= mask;
		outb(slave_mask, SLAVE_8259_MASK_PORT);
	} 
	else {
		master_mask &= mask;
		outb(master_mask, MASTER_8259_MASK_PORT);
	}

	restore_flags(flags);
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
	uint8_t mask = 1 << (irq_num & 7);
	uint32_t flags;
	cli_and_save(flags);
	if (irq_num & 8) {
		slave_mask |= mask;
		outb(slave_mask, SLAVE_8259_MASK_PORT);
	} 
	else {
		master_mask |= mask;
		outb(master_mask, MASTER_8259_MASK_PORT);
	}

	restore_flags(flags);
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {

	/* mask first then send EOI */
	uint8_t mask = 1 << (irq_num & 7);
	uint32_t flags;

	cli_and_save(flags);

	if (irq_num & 8) {
		slave_mask |= mask;
		outb(slave_mask, SLAVE_8259_MASK_PORT);
		outb(EOI+(irq_num&7), SLAVE_8259_PORT);
	} 
	else {
		master_mask |= mask;
		outb(master_mask, MASTER_8259_MASK_PORT);
		outb(EOI+(irq&7), MASTER_8259_PORT);
	}

	restore_flags(flags);

}
