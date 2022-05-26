#model_matrix matrix4 96
#view_matrix matrix4 100
#projection_matrix matrix4 104
#fog_value vector 108

mul r0, iPos.y, #model_matrix[1]
mad r0, iPos.x, #model_matrix[0], r0
mad r0, iPos.z, #model_matrix[2], r0
mad r0, iPos.w, #model_matrix[3], r0

mul r1, r0.y, #view_matrix[1]
mad r1, r0.x, #view_matrix[0], r1
mad r1, r0.z, #view_matrix[2], r1
mad r0, r0.w, #view_matrix[3], r1

mul r1, r0.y, #projection_matrix[1]
mad r1, r0.x, #projection_matrix[0], r1
mad r1, r0.z, #projection_matrix[2], r1
mad r0, r0.w, #projection_matrix[3], r1

; oPos.xyz = r0.xyz / r0.w
rcp r1.x, r0.w
mul oPos.xyz, r0, r1.x
mov oPos.w, r0

mov oD0, iDiffuse

mov oFog.yw, #fog_value.yw
