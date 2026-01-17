; A vertex shader otherwise same as the passthrough shader except z-coordinate
; is replaced by the reciprocal of z calculated by the clamping RCC instruction
; and xy are multiplied by that reciprocal.

rcc r0.x, iPos.z
mul oPos.xy, iPos.xy, r0.x
mov oPos.z, r0.x
mov oPos.w, iPos.w
mov oDiffuse, iDiffuse
mov oSpecular, iSpecular
mov oFog, iFog
mov oPts, iPts
mov oBackDiffuse, iBackDiffuse
mov oBackSpecular, iBackSpecular
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
