	.extern	g_xt_vbl_pending

	align 2
.global	irq_vbl

irq_vbl:
	clr.w	g_xt_vbl_pending
	rte
