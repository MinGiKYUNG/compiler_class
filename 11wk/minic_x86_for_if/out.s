    .section .rodata
fmt_print_int:
    .string "%d\n"
fmt_scanf_int:
    .string "%d"
    .text
    .globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    leaq -4(%rbp), %rsi
    leaq fmt_scanf_int(%rip), %rdi
    movl $0, %eax
    call scanf
    movl $0, %eax
    movl %eax, -8(%rbp)
.Lbegin_0:
    movl -8(%rbp), %eax
    pushq %rax
    movl -4(%rbp), %eax
    movl %eax, %ebx
    popq %rax
    cmpl %ebx, %eax
    setl %al
    movzbl %al, %eax
    cmpl $0, %eax
    je .Lend_1
    movl -8(%rbp), %eax
    movl %eax, %esi
    leaq fmt_print_int(%rip), %rdi
    movl $0, %eax
    call printf
    movl -8(%rbp), %eax
    pushq %rax
    movl $1, %eax
    movl %eax, %ebx
    popq %rax
    addl %ebx, %eax
    movl %eax, -8(%rbp)
    jmp .Lbegin_0
.Lend_1:
    movl $0, %eax
    leave
    ret
