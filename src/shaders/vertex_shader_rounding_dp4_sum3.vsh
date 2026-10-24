; Outputs (DP4((a, b, t2, 0), (1, 1, 1, 0)) - c) * scale + offset as screen z. c is the IEEE round-to-nearest value of
; a + b + t2, and is subtracted with a separate ADD so that it does not take part in the DP4's accumulation. The ADD and
; the final MAD are exact for the values used by the tests.

; {a, c, 0, scale}
#a_c_scale vector 96
; {b, -1, t2, offset}
#b_neg_one_t2_offset vector 97

mov oPos.xyw, iPos
mov r1, #b_neg_one_t2_offset
mov r2.x, #a_c_scale.x
mov r2.y, r1.x
mov r2.z, r1.z
mov r2.w, #a_c_scale.z
mov r3.xyz, -r1.y
mov r3.w, #a_c_scale.z
dp4 r0.x, r2, r3
add r0.x, r0.x, -#a_c_scale.y
mad oPos.z, r0.x, #a_c_scale.w, r1.w
mov oDiffuse, iDiffuse
