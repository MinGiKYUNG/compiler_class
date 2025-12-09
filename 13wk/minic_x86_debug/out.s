    .text
    .globl add
add:
    pushq %rbp
    movq %rsp, %rbp
    subq $24, %rsp
    movq %rdi, -8(%rbp)   # param a
    movq %rsi, -16(%rbp)   # param b
    # var s
    movq -16(%rbp), %rax
    pushq %rax
    movq -8(%rbp), %rax
    popq %rcx
    addq %rcx, %rax    # lhs + rhs
    movq %rax, -24(%rbp)   # s = rax
    movq -24(%rbp), %rax
    jmp .Lend_add
.Lend_add:
    leave
    ret

    .globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $24, %rsp
    # var x
    # var y
    # var result
    movq $3, %rax
    movq %rax, -8(%rbp)   # x = rax
    movq $4, %rax
    movq %rax, -16(%rbp)   # y = rax
    movq -8(%rbp), %rax
    pushq %rax
    movq -16(%rbp), %rax
    pushq %rax
    popq %rsi
    popq %rdi
    call add
    movq %rax, -24(%rbp)   # result = rax
    movq -24(%rbp), %rax
    jmp .Lend_main
.Lend_main:
    leave
    ret

