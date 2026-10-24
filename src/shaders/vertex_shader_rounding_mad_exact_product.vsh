; Outputs (a * m + b - c) * scale + offset as screen z, computing a * m + b with a single MAD. m is normally 1, so that
; only the MAD's addition can round. c is the IEEE round-to-nearest value of a * m + b. The subtraction of c and the
; final MAD are exact for the values used by the tests.

; {a, c, 0, scale}
#a_c_scale vector 96
; {b, -m, 0, offset}
#b_neg_one_offset vector 97

mov oPos.xyw, iPos
mov r1, #b_neg_one_offset
mad r0.x, #a_c_scale.x, -r1.y, r1.x
add r0.x, r0.x, -#a_c_scale.y
mad oPos.z, r0.x, #a_c_scale.w, r1.w
mov oDiffuse, iDiffuse
