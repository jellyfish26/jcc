.intel_syntax noprefix
.data
.LC25:
  .string "(2 * 3 > 2 * 2) + (2 * 3 <= 2 * 2)"
.data
.LC24:
  .string "(3 <= 3) + (3 < 3) + (4 > 4) + (4 >= 4)"
.data
.LC23:
  .string "(1 == 1) + (0 != 1)"
.data
.LC22:
  .string "0 != 1"
.data
.LC21:
  .string "0 <= 1"
.data
.LC20:
  .string "0 >= 1"
.data
.LC19:
  .string "1 <= 0"
.data
.LC18:
  .string "1 >= 0"
.data
.LC17:
  .string "0 < 1"
.data
.LC16:
  .string "0 > 1"
.data
.LC15:
  .string "1 < 0"
.data
.LC14:
  .string "1 > 0"
.data
.LC13:
  .string "1 >= 1"
.data
.LC12:
  .string "1 <= 1"
.data
.LC11:
  .string "1 == 1"
.data
.LC10:
  .string "+2 + +2 - -2"
.data
.LC9:
  .string "-2 + 3"
.data
.LC8:
  .string "-2 + 3"
.data
.LC7:
  .string "((2 * 3 - 2) / 2) * (1 + 3 * 2)"
.data
.LC6:
  .string "(2 + 3) * (3 - 2)"
.data
.LC5:
  .string "2 * 2 / 2 + 8 / 2 + 6 - 2"
.data
.LC4:
  .string "2 * 2 + 1 * 1"
.data
.LC3:
  .string "8 / 2"
.data
.LC2:
  .string "2 * 2"
.data
.LC1:
  .string "5 - 2"
.data
.LC0:
  .string "2 + 3"
.global main
.text
main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov rax, offset .LC0
  push rax
  pop rax
  push rax
  push 2
  push 3
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 5
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC1
  push rax
  pop rax
  push rax
  push 5
  push 2
  pop rdi
  pop rax
  sub eax, edi
  push rax
  push 3
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC2
  push rax
  pop rax
  push rax
  push 2
  push 2
  pop rdi
  pop rax
  imul eax, edi
  push rax
  push 4
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC3
  push rax
  pop rax
  push rax
  push 8
  push 2
  pop rdi
  pop rax
  cdq
  idiv edi
  mov rdx, rax
  mov eax, edx
  push rax
  push 4
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC4
  push rax
  pop rax
  push rax
  push 2
  push 2
  pop rdi
  pop rax
  imul eax, edi
  push rax
  push 1
  push 1
  pop rdi
  pop rax
  imul eax, edi
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 5
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC5
  push rax
  pop rax
  push rax
  push 2
  push 2
  push 2
  pop rdi
  pop rax
  cdq
  idiv edi
  mov rdx, rax
  mov eax, edx
  push rax
  pop rdi
  pop rax
  imul eax, edi
  push rax
  push 8
  push 2
  pop rdi
  pop rax
  cdq
  idiv edi
  mov rdx, rax
  mov eax, edx
  push rax
  push 6
  push 2
  pop rdi
  pop rax
  sub eax, edi
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 10
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC6
  push rax
  pop rax
  push rax
  push 2
  push 3
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 3
  push 2
  pop rdi
  pop rax
  sub eax, edi
  push rax
  pop rdi
  pop rax
  imul eax, edi
  push rax
  push 5
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC7
  push rax
  pop rax
  push rax
  push 2
  push 3
  pop rdi
  pop rax
  imul eax, edi
  push rax
  push 2
  pop rdi
  pop rax
  sub eax, edi
  push rax
  push 2
  pop rdi
  pop rax
  cdq
  idiv edi
  mov rdx, rax
  mov eax, edx
  push rax
  push 1
  push 3
  push 2
  pop rdi
  pop rax
  imul eax, edi
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  pop rdi
  pop rax
  imul eax, edi
  push rax
  push 14
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC8
  push rax
  pop rax
  push rax
  push 0
  push 2
  pop rdi
  pop rax
  sub eax, edi
  push rax
  push 3
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC9
  push rax
  pop rax
  push rax
  push 0
  push 2
  pop rdi
  pop rax
  sub eax, edi
  push rax
  push 3
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC10
  push rax
  pop rax
  push rax
  push 0
  push 2
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 0
  push 2
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 0
  push 2
  pop rdi
  pop rax
  sub eax, edi
  push rax
  pop rdi
  pop rax
  sub eax, edi
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 6
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC11
  push rax
  pop rax
  push rax
  push 1
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  sete al
  movzx rax, al
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC12
  push rax
  pop rax
  push rax
  push 1
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  setle al
  movzx rax, al
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC13
  push rax
  pop rax
  push rax
  push 1
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  setge al
  movzx rax, al
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC14
  push rax
  pop rax
  push rax
  push 1
  push 0
  pop rdi
  pop rax
  cmp eax, edi
  setg al
  movzx rax, al
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC15
  push rax
  pop rax
  push rax
  push 1
  push 0
  pop rdi
  pop rax
  cmp eax, edi
  setl al
  movzx rax, al
  push rax
  push 0
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC16
  push rax
  pop rax
  push rax
  push 0
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  setg al
  movzx rax, al
  push rax
  push 0
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC17
  push rax
  pop rax
  push rax
  push 0
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  setl al
  movzx rax, al
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC18
  push rax
  pop rax
  push rax
  push 1
  push 0
  pop rdi
  pop rax
  cmp eax, edi
  setge al
  movzx rax, al
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC19
  push rax
  pop rax
  push rax
  push 1
  push 0
  pop rdi
  pop rax
  cmp eax, edi
  setle al
  movzx rax, al
  push rax
  push 0
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC20
  push rax
  pop rax
  push rax
  push 0
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  setge al
  movzx rax, al
  push rax
  push 0
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC21
  push rax
  pop rax
  push rax
  push 0
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  setle al
  movzx rax, al
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC22
  push rax
  pop rax
  push rax
  push 0
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  setne al
  movzx rax, al
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC23
  push rax
  pop rax
  push rax
  push 1
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  sete al
  movzx rax, al
  push rax
  push 0
  push 1
  pop rdi
  pop rax
  cmp eax, edi
  setne al
  movzx rax, al
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 2
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC24
  push rax
  pop rax
  push rax
  push 3
  push 3
  pop rdi
  pop rax
  cmp eax, edi
  setle al
  movzx rax, al
  push rax
  push 3
  push 3
  pop rdi
  pop rax
  cmp eax, edi
  setl al
  movzx rax, al
  push rax
  push 4
  push 4
  pop rdi
  pop rax
  cmp eax, edi
  setg al
  movzx rax, al
  push rax
  push 4
  push 4
  pop rdi
  pop rax
  cmp eax, edi
  setge al
  movzx rax, al
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 2
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  mov rax, offset .LC25
  push rax
  pop rax
  push rax
  push 2
  push 3
  pop rdi
  pop rax
  imul eax, edi
  push rax
  push 2
  push 2
  pop rdi
  pop rax
  imul eax, edi
  push rax
  pop rdi
  pop rax
  cmp eax, edi
  setg al
  movzx rax, al
  push rax
  push 2
  push 3
  pop rdi
  pop rax
  imul eax, edi
  push rax
  push 2
  push 2
  pop rdi
  pop rax
  imul eax, edi
  push rax
  pop rdi
  pop rax
  cmp eax, edi
  setle al
  movzx rax, al
  push rax
  pop rdi
  pop rax
  add eax, edi
  push rax
  push 1
  pop rdi
  pop rsi
  pop rdx
  call check
  push rax
  push 0
  pop rax
  mov rsp, rbp
  pop rbp
  ret
  mov rsp, rbp
  pop rbp
  ret 
