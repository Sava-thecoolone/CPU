; fill the screen

; initialize registers
MOV 2 -> %0 ; clear the screen
MOV 1 -> A
MOV 1 -> B

MOV A -> %[B] ; fill a pixel at the address A
ADD A, 1 -> A ; increment register A

JZ A, 0  ; if looped back to the beginning, jump to the start (clear the screen)
JMP 3    ; loop back