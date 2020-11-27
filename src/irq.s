	.extern	vbl_ticks

.global	irq_vbl

irq_vbl:
	addi.w	#1, vbl_ticks
	rts
