; A vertex shader that performs palette-indexed position lookup and 3D perspective projection.

#model_matrix matrix4 96
#view_matrix matrix4 100
#projection_matrix matrix4 104
#index_scale vector 110

; Blend 3 palette positions using indices from iDiffuse and barycentric weights from iSpecular
mul r1.x, iDiffuse.x, #index_scale.x
arl a0.x, r1.x
mul r0, iSpecular.x, c[A0+112]

mul r1.x, iDiffuse.y, #index_scale.x
arl a0.x, r1.x
mad r0, iSpecular.y, c[A0+112], r0

mul r1.x, iDiffuse.z, #index_scale.x
arl a0.x, r1.x
mad r0, iSpecular.z, c[A0+112], r0

mov r0.w, #index_scale.w

%matmul4x4 r1 r0 #model_matrix
%matmul4x4 r0 r1 #view_matrix
%matmul4x4 r1 r0 #projection_matrix

rcp r2.x, r1.w
mul oPos.xyz, r1, r2.x
mov oPos.w, r1.w

mov oDiffuse, iDiffuse
mov oSpecular, iSpecular
mov oBackDiffuse, iBackDiffuse
mov oBackSpecular, iBackSpecular
mov oFog, r12.w
mov oPts, iPts
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
