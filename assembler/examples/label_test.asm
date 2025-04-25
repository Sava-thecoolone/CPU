MOV 10 -> $0  ; write 10 into RAM at address 0
MOV ~end -> B ; set B as the address of the end label
JMP ~what     ; jump to ~what

~what
MOV $0 -> A ; write from RAM at address 0 into A (address 0 was set to 10 previously)
JMP B       ; jump to B, which is the ~end label

~end ; send the program counter into the void