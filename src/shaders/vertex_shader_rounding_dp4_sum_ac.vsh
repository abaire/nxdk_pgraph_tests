; Outputs (a - c + b) * scale + offset as screen z, computing a - c + b as a single DP4 of (a, c, b, 0) and
; (1, -1, 1, 0). All products are exact, and a - c and (a - c) + b are exact for the values used by the tests, so the
; result only deviates from the exact value if the accumulation adds b before c. The final MAD is exact.

#a_c_scale vector 96
#b_neg_one_offset vector 97

mov oPos.xyw, iPos
mov r1, #b_neg_one_offset
mov r2.x, #a_c_scale.x
mov r2.y, #a_c_scale.y
mov r2.z, r1.x
mov r2.w, #a_c_scale.z
mov r3.xz, -r1.y
mov r3.y, r1.y
mov r3.w, r1.z
dp4 r0.x, r2, r3
mad oPos.z, r0.x, #a_c_scale.w, r1.w
mov oDiffuse, iDiffuse
