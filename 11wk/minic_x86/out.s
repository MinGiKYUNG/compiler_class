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
    subq $16, %rsp
    movl $10, %eax
    movl %eax, -8(%rbp)
    leaq -4(%rbp), %rsi
    leaq fmt_scanf_int(%rip), %rdi
    movl $0, %eax
    call scanf
    movl -4(%rbp), %eax
    pushq %rax
    movl -8(%rbp), %eax
    movl %eax, %ebx
    popq %rax
    addl %ebx, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    movl %eax, %esi
    leaq fmt_print_int(%rip), %rdi
    movl $0, %eax
    call printf
    movl $0, %eax
    leave
    ret
