# ===============================================================
#
# R63T �X�^�[�g�A�b�v
#
# ===============================================================
	.text
	.global _start
	.type	_start,@function
_start:

# �X�^�b�N�̐ݒ�iSP�F__stack�AUSP�F__stack - 256�j
	mov.l	#__stack, r0
	mvtc	r0, isp
	sub		#256,r0
	mvtc	r0, usp

# ���荞�݃x�N�^�̐ݒ�
	.extern _interrupt_vectors
	mov.l	#_interrupt_vectors, r5
	mvtc	r5,intb

	mov.l	#0x100, r5
	mvtc	r5,fpsw

# I���W�X�^��ݒ肵�A���荞�݂�������
	mov.l	#0x00010000, r5
	mvtc	r5,psw

# PM���W�X�^��ݒ肵�A���[�U���[�h�Ɉڍs����
	mvfc	psw,r1
	or		#0x100000, r1
	push.l	r1

# U���W�X�^���Z�b�g���邽�߂�RTE���߂����s����
	mvfc	pc,r1
	add		#0x0a,r1
	push.l	r1
	rte
	nop

# �����l�t���ϐ��̏�����
	mov	#__datastart, r1
	mov	#__romdatastart, r2
	mov	#__romdatacopysize, r3
	smovf

# .bss �Z�N�V�����̏�����
	mov	#__bssstart, r1
	mov	#0, r2
	mov	#__bsssize, r3
	sstr.l

# init() �֐�����J�n
	.extern	_init
	bra		_init

	.global	_rx_run_preinit_array
	.type	_rx_run_preinit_array,@function
_rx_run_preinit_array:
	mov	#__preinit_array_start,r1
	mov	#__preinit_array_end,r2
	bra.a	_rx_run_inilist

	.global	_rx_run_init_array
	.type	_rx_run_init_array,@function
_rx_run_init_array:
	mov	#__init_array_start,r1
	mov	#__init_array_end,r2
	mov	#4, r3
	bra.a	_rx_run_inilist

	.global	_rx_run_fini_array
	.type	_rx_run_fini_array,@function
_rx_run_fini_array:
	mov	#__fini_array_start,r2
	mov	#__fini_array_end,r1
	mov	#-4, r3

# ���������X�g
_rx_run_inilist:
next_inilist:
	cmp	r1,r2
	beq.b	done_inilist
	mov.l	[r1],r4
	cmp	#-1, r4
	beq.b	skip_inilist
	cmp	#0, r4
	beq.b	skip_inilist
	pushm	r1-r3
	jsr	r4
	popm	r1-r3

skip_inilist:
	add	r3,r1
	bra.b	next_inilist
done_inilist:
	rts

	.global	_set_intr_level
_set_intr_level:
	and		#15,r1
	shll	#24,r1
	or		#0x100000,r1
	mvtc	r1,psw
	rts

	.global _exit
_exit:
	wait
	bra		_exit
