; A vertex shader that passes through all parameters except colors with no
; manipulation. Colors are multiplied by uniforms.
; Note that iWeight and iNormal are ignored as they'd generally be used in
; per-vertex lighting.

#diffuse_multiplier vector 120
#specular_multiplier vector 121
#back_diffuse_multiplier vector 122
#back_specular_multiplier vector 123


mov oPos, iPos
mul oDiffuse, iDiffuse, #diffuse_multiplier
mul oSpecular, iSpecular, #specular_multiplier
mov oFog, iFog
mov oPts, iPts
mul oBackDiffuse, iBackDiffuse, #back_diffuse_multiplier
mul oBackSpecular, iBackSpecular, #back_specular_multiplier
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
