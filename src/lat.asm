.syntax unified
.cpu cortex-r5
.thumb

.global preplatencyarr
.type preplatencyarr, %function
preplatencyarr:
    push {r4, r5, r6, lr}
    movs r2, #0              @ i = 0
1:
    ldr r3, [r0, r2, lsl #2] @ r3 = arr[i]
    lsls r3, r3, #2
    adds r3, r3, r0          @ r3 = &arr[arr[i]]
    str r3, [r0, r2, lsl #2] @ arr[i] = pointer
    adds r2, r2, #1
    cmp r2, r1
    bne 1b
    pop {r4, r5, r6, pc}
    
.global latencytest
.type latencytest, %function
latencytest:
    push {r4, r5, lr}
    ldr r2, [r1]       @ r2 = *arr
    movs r3, #0        @ accumulator
1:
    ldr r2, [r2]       @ r2 = *r2
    adds r3, r3, r2
    subs r0, r0, #1
    bne 1b
    mov r0, r3         @ ·µ»ØÖµ
    pop {r4, r5, pc}
