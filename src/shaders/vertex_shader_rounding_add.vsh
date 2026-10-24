; Outputs (a + b - c) * scale + offset as screen z, where c is the IEEE round-to-nearest value of a + b, so the
; difference from the offset reveals how ADD rounded. The subtraction of c and the final MAD are exact for the values
; used by the tests.

#a_c_scale vector 96
#b_neg_one_offset vector 97

mov oPos.xyw, iPos
mov r1, #b_neg_one_offset
add r0.x, #a_c_scale.x, r1.x
add r0.x, r0.x, -#a_c_scale.y
mad oPos.z, r0.x, #a_c_scale.w, r1.w
mov oDiffuse, iDiffuse
