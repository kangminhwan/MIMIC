.386
.MODEL flat, C

.stack 4096

.data

.code
start:
	GetStackPointerX86 proc
		lea eax, [esp]
		ret
	GetStackPointerX86 endp

end start