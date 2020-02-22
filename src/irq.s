	.extern	vbl_ticks

.global	vbl_isr

vbl_isr:
	addi.w	#1, vbl_ticks
	rts
