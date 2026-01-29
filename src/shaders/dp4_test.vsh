; A vertex shader that sets oDiffuse to the DP4 of:
;  r - iWeight and #weight_vector
;  g - iDiffuse and #diffuse_vector
;  b - iSpecular and #specular_vector
;  a - #one_constant.x
;
; All other values are passed through unchanged.

#weight_vector      vector 96  ; A vector to be dot'ed with the weight in
#diffuse_vector     vector 97  ; A vector to be dot'ed with the diffuse in
#specular_vector    vector 98  ; A vector to be dot'ed with the specular in
#one_constant       vector 99  ; A vector containing all 1.0 values.

mov oPos, iPos

dp4 oDiffuse.r, iWeight, #weight_vector
dp4 oDiffuse.g, iDiffuse, #diffuse_vector
dp4 oDiffuse.b, iSpecular, #specular_vector
mov oDiffuse.a, #one_constant.a

mov oSpecular, iSpecular
mov oFog, iFog
mov oPts, iPts
mov oBackDiffuse, iBackDiffuse
mov oBackSpecular, iBackSpecular
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
