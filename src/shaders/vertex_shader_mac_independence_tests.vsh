; A vertex shader that effectively adds the RGB components of the diffuse and
; specular inputs together and outputs them as diffuse. Then it
; takes the R component of the diffuse, performs a reciprocal on it twice and
; writes it as specular.
; All other values are passed through.

; Use oPos to make this more interesting since emulators will need to handle
; the R12 aliasing.
mov oPos, iDiffuse
sge oPos.a, R0.a, R0.a ; Force alpha to 1.0
add oPos.xyz, R12, iSpecular + add R0.xyz, R12, iSpecular
; Use the R0 calculation to ensure that it is not corrupted.
mov oDiffuse, R0

; Do the same with the ILU
rcp oPos.r, iDiffuse.r
rcp oPos.r, R12.r + rcp R1.r, R12.r
mov oSpecular, R1.rrr

; Pass through all other values
mov oFog, iFog
mov oPts, iPts
mov oBackDiffuse, iBackDiffuse
mov oBackSpecular, iBackSpecular
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3

mov oPos, iPos
