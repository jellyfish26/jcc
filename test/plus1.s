.globl main
.text
.type main, @function
main:
  push %rbp
  mov %rsp, %rbp
  mov $2, %rax
  mov %rax, %rdi
  mov $1, %rax
  add %rdi, %rax
  mov %rbp, %rsp
  pop %rbp
  ret
