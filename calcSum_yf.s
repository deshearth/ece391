calcSum_yf:
.global calcSum_yf
.type calcSum_yf @function
.code32
calcSum_yf:
  pushl %ebp                                                                   
	movl %esp, %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	 
	movl 8(%ebp), %ebx                 # ebx = array[]
	movl 12(%ebp), %ecx                # ecx = arrayLen
	 
	xor $edi, %edi                     # clear edi
#pushl %edi 		# save as local variable, no need to save, since it is always 0 in 
# each frame
	 
	cmpl $0, %ecx
	jle  Done
	 
	pushl %eax		# caller save
	pushl %ecx
	pushl %edx

	movl %ecx, %edx
	subl $1, %edx                     #edx = arrayLen -1
	pushl %edx		# push first arg array, 
	push %ebx			# push second arg arrayLen-1
	call calcSum_yf
	addl $8, %esp

	popl %edx
	popl %ecx
	popl %eax

	addl %eax, %edi           # sum += calcSum(array, arrayLen-1)
	addl  (%ebx, %edx, 4), %edi       #/ sum += array[arrayLen - 1]
	 
	 
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

