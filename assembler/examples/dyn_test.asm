; dynamic addressing test
; writes value from B to screen position specified by A

; initialize registers
MOV 1 -> A   ; screen position (e.g., column 10)
MOV 10 -> B  ; value to write to screen

; write A to screen at position B
MOV B -> %[A]  ; dynamic addressing - uses B as value
