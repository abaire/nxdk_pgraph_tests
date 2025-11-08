; A vertex shader that passes through all parameters except diffuse with no
; manipulation. Diffuse is set to iPts multiplied by the first uniform.

#diffuse_multiplier vector 120


mov oPos, iPos
mul oDiffuse, iPts, #diffuse_multiplier
mov oSpecular, iSpecular
mov oFog, iFog
mov oPts, iPts
mov oBackDiffuse, iBackDiffuse
mov oBackSpecular, iBackSpecular
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
