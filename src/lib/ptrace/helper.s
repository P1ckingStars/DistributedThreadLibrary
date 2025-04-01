	.file	"helper.cpp"
	.text
	.globl	_Z9injectionv
	.type	_Z9injectionv, @function
_Z9injectionv:
    syscall
	movq	$0, %rax
	movl	(%rax), %eax
    
.LFE0:
	.size	_Z9injectionv, .-_Z9injectionv
	.ident	"GCC: (GNU) 14.2.1 20240910"
	.section	.note.GNU-stack,"",@progbits
