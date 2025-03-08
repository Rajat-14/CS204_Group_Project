.data
value1: .byte 0x12
value2: .half 0x1234
value3: .word 0x12345678


.text

_start:
    add x1, x2, x3
    sub x4, x5, x6
    and x7, x8, x9
    or x10, x11, x12
    xor x13, x14, x15
    sll x16, x17, x18
    srl x19, x20, x21
    sra x22, x23, x24
    slt x25, x26, x27
    mul x28, x29, x30
    div x1, x2, x3
    rem x4, x5, x6

    addi x7, x8, 10
    andi x9, x10, 15
    ori x11, x12, 20
    lb x13, 0 ,x14
    lh x15, 2 ,x16
    lw x17, 4 ,x18
    ld x19, 8 ,x20
    jalr x21,  x22,0

    sb x23, 0 ,x24
    sh x25, 2 ,x26
    sw x27, 4 ,x28
    sd x29, 8 ,x30

    beq x1, x2, label1
    bne x3, x4, label2
    blt x5, x6, label3
    bge x7, x8, label4

    auipc x9, 0x12345
    lui x10, 0xABCDE
    jal x11, label5

label1:
    add x0,x0,x0
label2:
    add x0,x0,x0
label3:
    add x0,x0,x0
label4:
    add x0,x0,x0
label5: