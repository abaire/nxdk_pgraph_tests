#model_matrix matrix4 96
#view_matrix matrix4 100
#projection_matrix matrix4 104
#fog_value vector 120

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

; Elements are intentionally set in multiple instructions.
mov oFog.x, #fog_value.x
mov oFog.y, #fog_value.y
mov oFog.z, #fog_value.z
mov oFog.w, #fog_value.w
