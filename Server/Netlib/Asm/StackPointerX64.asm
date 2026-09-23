
; 해당 함수의 호출 하는곳의 스택을 조사하기위해 x86은 esp, x64는 rsp 를 리턴합니다.
; Base Stack 으로부터 얼마 만큼의 스택이 사용되어졌는지를 알기 위해서입니다.
; 01. Common 폴더에 Utility.h 에 GetStackPointerInfo() 함수를 호출하면
; 컴파일 된 Platform 에 맞춰서 함수를 콜해줍니다.

.code
	GetStackPointerX64 proc
		lea rax, [rsp]
		ret
	GetStackPointerX64 endp

end