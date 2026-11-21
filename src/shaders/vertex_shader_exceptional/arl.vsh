; Loads A0 from the x component of v9 (texcoord 0) and emits c[A0 + 0] as the diffuse color, mirroring
; vsh_a0_relative.vsh in nxdk_low_level_nv2a_tests.
mov oPos, iPos
arl a0, iTex0
mov oDiffuse, c[A0 + 0]
