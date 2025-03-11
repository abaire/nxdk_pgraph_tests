; A vertex shader that performs transformation and applies a basic, colorless
; diffuse lighting calculation, passing through other values unmodified.
;

#model_matrix matrix4 96
#view_matrix matrix4 100
#projection_matrix matrix4 104
#camera_pos vector 108
#light_direction vector 109
#zero_constant vector 114
; -1, -0.5, 0.5, 1
#basic_constants vector 115
#invalid_color vector 116

; -- Transform the vertex. -----------------
%matmul4x4 r0 iPos #model_matrix
%matmul4x4 r1 r0 #view_matrix
%matmul4x4 r0 r1 #projection_matrix

; oPos.xyz = r0.xyz / r0.w
rcp r1.x, r0.w
mul oPos.xyz, r0, r1.x
mov oPos.w, r0
; ------------------------------------------


; Transform the normal by the model matrix and normalize --
%matmul4x4 r2 iNormal #model_matrix
%norm3 r2 r2 r0
; ---------------------------------------------------------

; Normalize the light direction
%norm3 r3 #light_direction r0

; Calculate contribution from directional light source
; directional_contribution = r0.x = max(normal <dot> light_direction, 0)
dp3 r0.x, r2, r3
max oDiffuse.rgb, r0.x, #zero_constant.x

mov oSpecular, iSpecular

; Set back diffuse/specular to magenta to make it clear that they are not being
; calculated by this shader.
mov oBackDiffuse, #invalid_color
mov oBackSpecular, #invalid_color

mov oFog, r12.w
mov oPts, iPts
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
