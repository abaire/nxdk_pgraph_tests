; A vertex shader that performs transformation and applies some of the lighting
; calculations that would be done by the fixed function pipeline.
;

#model_matrix matrix4 96
#view_matrix matrix4 100
#projection_matrix matrix4 104
#camera_pos vector 108
#light_vector vector 109
#zero_constant vector 110
; -1, -0.5, 0.5, 1
#basic_constants vector 111

#invalid_color vector 120
#light_ambient vector 121
#light_diffuse vector 122
#light_specular vector 123
#material_shininess vector 124
#scene_ambient vector 125

; -- Transform the vertex. -----------------
%matmul4x4 r0 iPos #model_matrix
%matmul4x4 r1 r0 #view_matrix
%matmul4x4 r0 r1 #projection_matrix

; oPos.xyz = r0.xyz / r0.w
rcp r1.x, r0.w
mul oPos.xyz, r0, r1.x
mov oPos.w, r0
; ------------------------------------------


; -- Transform the normal by the model matrix and normalize -----
%matmul4x4 r2 iNormal #model_matrix
%norm3 r2 r2 r11
; ---------------------------------------------------------------

; For an infinite light, VPpli is the normalized position of the light
%norm3 r3 #light_vector r11

; Calculate contribution from directional light source
; directional_contribution = r0.x = max(normal <dot> light_vector, 0)
dp3 r0.x, r2, r3
max r0.x, r0.x, #zero_constant.x

; Diffuse =
;  #scene_ambient.rgb +
;  #light_ambient.rgb + (directional_contribution * #light_diffuse.rgb)
mul r1, #light_diffuse.rgb, r0.x
add r1, r1, #scene_ambient.rgb
add oDiffuse, r1, #light_ambient.rgb

; -- Specular ---------------------------------------------------

; (normal <dot> half_angle) ^ specular_exponent * #light_specular.rgb

; -- Calculate infinite half vector ----

; Assume camera is infinitely distant (LOCAL_EYE = false)
; The half-angle vector in this case should be
;   norm(norm(VPpli) + (0, 0, -1))

mov r0.xy, #zero_constant
mov r0.z, #basic_constants.x
add r0, r3, r0
%norm3 r0 r0 r11

; r0.x = normal <dot> half_angle
dp3 r0, r2, r0

; pow(r0.x, #material_shininess.x)
mov r5.w, #material_shininess.x
mov r5.xy, r0.x
lit r0, r5

mul oSpecular, #light_specular.rgb, r0.zzz
mov oSpecular.a, iSpecular.a

; ---------------------------------------------------------------


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
