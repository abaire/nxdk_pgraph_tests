; A vertex shader that just passes through all parameters with no manipulation.
; Note that iWeight and iNormal are ignored as they'd generally be used in
; per-vertex lighting.

mov oPos, iPos
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
