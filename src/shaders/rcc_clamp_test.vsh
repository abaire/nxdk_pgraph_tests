; A vertex shader otherwise same as the passthrough shader except w-coordinate
; is replaced by input z divided by input w where reciprocal of w is calculated
; using the clamping RCC instruction.

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
