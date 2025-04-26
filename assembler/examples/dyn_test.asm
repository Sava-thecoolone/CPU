; dynamic addressing test
; writes value from A to screen position specified by B

; initialize registers
MOV #1 -> A   ; value to write to screen
MOV #10 -> B  ; screen position (e.g., column 10)

; write A to screen at position B
MOV B -> %[A]  ; dynamic addressing - uses A as value
