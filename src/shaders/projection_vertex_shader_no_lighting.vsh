; A vertex shader that performs transformation and passes through all other
; vertex attributes without modification.

#model_matrix matrix4 96
#view_matrix matrix4 100
#projection_matrix matrix4 104

; -- Transform the vertex. -----------------
%matmul4x4 r0 iPos #model_matrix
%matmul4x4 r1 r0 #view_matrix
%matmul4x4 r0 r1 #projection_matrix

; oPos.xyz = r0.xyz / r0.w
rcp r1.x, r0.w
mul oPos.xyz, r0, r1.x
mov oPos.w, r0
; ------------------------------------------


mov oDiffuse, iDiffuse
mov oSpecular, iSpecular
mov oBackDiffuse, iBackDiffuse
mov oBackSpecular, iBackSpecular
mov oFog, iFog
mov oPts, iPts
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
