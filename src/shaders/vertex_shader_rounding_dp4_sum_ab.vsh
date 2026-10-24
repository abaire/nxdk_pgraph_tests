; Outputs (a + b - c) * scale + offset as screen z, computing a + b - c as a single DP4 of (a, b, c, 0) and
; (1, 1, -1, 0). All products are exact, so only the accumulation can round. c is the IEEE round-to-nearest value of
; a + b. The final MAD is exact for the values used by the tests.

#a_c_scale vector 96
#b_neg_one_offset vector 97

mov oPos.xyw, iPos
mov r1, #b_neg_one_offset
mov r2.x, #a_c_scale.x
mov r2.y, r1.x
mov r2.z, #a_c_scale.y
mov r2.w, #a_c_scale.z
mov r3.xy, -r1.y
mov r3.z, r1.y
mov r3.w, r1.z
dp4 r0.x, r2, r3
mad oPos.z, r0.x, #a_c_scale.w, r1.w
mov oDiffuse, iDiffuse
