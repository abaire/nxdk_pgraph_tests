; Outputs (a * b - c) * scale + offset as screen z, computing a * b - c with a single DP4 of (a, c, 0, 0) and
; (b, -1, 0, offset). c is the IEEE round-to-nearest value of a * b, so the difference from the offset reveals how DP4
; rounded. The final MAD is exact for the values used by the tests.

; {a, c, 0, scale}
#a_c_scale vector 96
; {b, -1, 0, offset}
#b_neg_one_offset vector 97

mov oPos.xyw, iPos
mov r1, #b_neg_one_offset
dp4 r0.x, #a_c_scale.xyzz, r1
mad oPos.z, r0.x, #a_c_scale.w, r1.w
mov oDiffuse, iDiffuse
