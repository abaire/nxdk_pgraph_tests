; Fixed function fog distance chain with P = c[96]: P * rcp(P.w).
mov oPos, v0
rcp r0.x, c[96].w
mul r10, c[96], r0.x

; Shared epilogue, described in VertexShaderExceptionalMACTests. Lane i of r11 is 1 when r10.i has the same bits as
; c[100].i and 0 otherwise. c[102] selects the lane shown, so only these exact 0 or 1 values pass through DP4.
sge r9, r10, c[100]
sge r11, c[100], r10
mul r11, r9, r11
dp4 r11.x, r11, c[102]
mov r8, c[104]
mad oDiffuse, r11.xxxx, c[103], r8
