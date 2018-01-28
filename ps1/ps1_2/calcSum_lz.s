.text
.global calcSum_lz
.type calcSum_lz @function
.code32
calcSum_lz:
  pushl %ebp                                                                   
	movl %esp, %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	 
	movl 8(%ebp), %ecx                 # ecx = array[]
	movl 12(%ebp), %ebx                # ebx = arrayLen
	 
	movl $0, %edi                     # edi = sum
	 
	cmpl $0, %ebx
	jle  Done
	 
	movl %ebx, %edx
	subl $1, %edx                     #edx = arrayLen -1
	push %edx
	push %ecx
	call calcSum_lz
	popl %ecx
	popl %edx

	 
	addl %eax, %edi           # sum += calcSum(array, arrayLen-1)
	addl  (%ecx, %edx, 4), %edi       #/ sum += array[arrayLen - 1]
	 
	 
	movl %edi, %eax
	jmp RecurDone

Done:
	movl $0, %eax

RecurDone:
	popl %edi
	popl  %esi
	popl  %ebx
	leave
	ret

