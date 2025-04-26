; A vertex shader that just passes through all parameters except for iPts with
; no manipulation.
; Note that iWeight and iNormal are ignored as they'd generally be used in
; per-vertex lighting and there is no output register for them.

mov oPos, iPos
mov oDiffuse, iDiffuse
mov oSpecular, iSpecular
mov oFog, iFog
mov oBackDiffuse, iBackDiffuse
mov oBackSpecular, iBackSpecular
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
