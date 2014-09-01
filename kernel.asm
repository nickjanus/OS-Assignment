;kernel.asm
;Michael Black, 2007

;kernel.asm contains assembly functions that you can use in your kernel

	.global _putInMemory

;void putInMemory (int segment, int address, char character)
_putInMemory:
	mov bp,sp
	push ds
	mov ax,[bp+2]
	mov si,[bp+4]
	mov cl,[bp+6]
	mov ds,ax
	mov [si],cl
	pop ds
	ret
