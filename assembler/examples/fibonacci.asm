; fibonacci sequence test
; calulates the fibonacci sequence

; initialize registers
MOV 1 -> A
MOV 1 -> B

; actual fibonacci stuff
ADD A, B -> $10 ; add both registers into RAM at address 10

; shift everything
MOV B -> A
MOV $10 -> B
JMP 2 ; jump back to command 3 (command addresses start at 0!!!)