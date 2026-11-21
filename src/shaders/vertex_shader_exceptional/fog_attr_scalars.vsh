; Fixed function fog distance chain with P = v1: rcp(P.w) (x), d2 = DP3(p, p) with p = P * rcp(P.w) (y),
; rsq(d2) (z) and d2 * rsq(d2) (w).
mov oPos, v0
rcp r10.x, v1.w
mul r1, v1, r10.x
dp3 r10.y, r1, r1
rsq r10.z, r10.y
mul r10.w, r10.y, r10.z

; Shared epilogue, described in VertexShaderExceptionalMACTests. Lane i of r11 is 1 when r10.i has the same bits as
; c[100].i and 0 otherwise. c[102] selects the lane shown, so only these exact 0 or 1 values pass through DP4.
sge r9, r10, c[100]
sge r11, c[100], r10
mul r11, r9, r11
dp4 r11.x, r11, c[102]
mov r8, c[104]
mad oDiffuse, r11.xxxx, c[103], r8
