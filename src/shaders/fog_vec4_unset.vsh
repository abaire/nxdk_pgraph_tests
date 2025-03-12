; Does a basic projection of the input position and passes through the
; diffuse color but leaves other registers unset.

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


mov oD0, iDiffuse
