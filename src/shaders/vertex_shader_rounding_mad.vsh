; Outputs (a * b - c) * scale + offset as screen z, computing a * b - c with a single MAD. c is the IEEE
; round-to-nearest value of a * b, so the difference from the offset reveals how MAD rounded. The final MAD is exact for
; the values used by the tests.

; {a, c, 0, scale}
#a_c_scale vector 96
; {b, -1, 0, offset}
#b_offset vector 97

mov oPos.xyw, iPos
mov r1, #b_offset
mad r0.x, #a_c_scale.x, r1.x, -#a_c_scale.y
mad oPos.z, r0.x, #a_c_scale.w, r1.w
mov oDiffuse, iDiffuse
