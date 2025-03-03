; A vertex shader performs transformation and applies some of the lighting
; calculations that would be done by the fixed function pipeline.
;

#model_matrix matrix4 96
#view_matrix matrix4 100
#projection_matrix matrix4 104
#camera_pos vector 108
#light_direction vector 109
#zero_constant vector 114

#invalid_color vector 120
#light_ambient vector 121
#light_diffuse vector 122
#light_specular vector 123

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
;%normalize3 r2 r2 r0
dp3 r0.x, r2, r2  ; r0.x = len(transformed_normal)^2
rsq r0.w, r0.x  ; r0.w = 1 / len(transformed_normal)
mul r2.xyz, r2, r0.w  ; r2.xyz = normalized(transformed_normal)
; ---------------------------------------------------------

; Normalize the light direction
%norm3 r3 #light_direction r0

; Calculate contribution from directional light source
; directional_contribution = r0.x = max(normal <dot> light_direction, 0)
dp3 r0.x, r2, r3
max r0.x, r0.x, #zero_constant.x

; Diffuse = #light_ambient.rgb + (directional_contribution * #light_diffuse.rgb)
; TODO: Specular using half infinite vector.
;   normalize(#camera_pos + #light_direction)
mul r1, #light_diffuse.rgb, r0.x
add oDiffuse, r1, #light_ambient.rgb

mov oSpecular, iSpecular

; Set back diffuse/specular to magenta to make it clear that they are not being
; calculated by this shader.
mov oBackDiffuse, #invalid_color
mov oBackSpecular, #invalid_color

mov oFog, iFog
mov oPts, iPts
mov oTex0, iTex0
mov oTex1, iTex1
mov oTex2, iTex2
mov oTex3, iTex3
