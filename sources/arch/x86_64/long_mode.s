global apollo_long_mode_entry
extern apollo_kernel_main

section .text
bits 64

apollo_long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov rsp, apollo_stack_top
    and rsp, -16    ; Ensure 16-byte alignment
    
    cld
    
    call initialize_fpu
    
    call apollo_kernel_main
    
apollo_halt_loop:
    cli
    hlt
    jmp apollo_halt_loop

initialize_fpu:
    clts
    
    fninit
    
    ret

section .bss
align 16
apollo_stack_bottom:
    resb 65536    ; 64KB kernel stack
apollo_stack_top: