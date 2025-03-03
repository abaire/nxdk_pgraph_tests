; A vertex shader that performs no transformation and sets the color to one of
; the input values based on the value of the first uniform.
;
; Note: This shader is intentionally unoptimized for readability.

#test_attribute vector     96  ; The input field that should be assigned to oDiffuse.
#one_constant vector       97  ; A vector containing all 1.0 values.
#unsupported_color vector  98  ; Color that should be returned for an invalid #test_attribute

; These fields provide the values that should be compared to test_attribute to
; select which input is assigned.
#weight_normal_diffuse_specular vector     99  ; {0, 1, 2, 3}
#fog_pts_backdiffuse_backspecular vector   100 ; {4, 5, 6, 7}
#tex0_tex1_tex2_tex3 vector                101 ; {8, 9, 10, 11}

; Set R0.x to the attribute index
mov r0.x, #test_attribute

; -- Set up multipliers for the branches ---------------------------------------

; r8 = weight | normal | diffuse | specular
sge r1, #weight_normal_diffuse_specular, r0.x
sge r2, r0.x, #weight_normal_diffuse_specular
mul r8, r1, r2

; r9 = fog | point_size | back_diffuse | back_specular
sge r1, #fog_pts_backdiffuse_backspecular, r0.x
sge r2, r0.x, #fog_pts_backdiffuse_backspecular
mul r9, r1, r2

; r10 = tex0 | tex1 | tex2 | tex3
sge r1, #tex0_tex1_tex2_tex3, r0.x
sge r2, r0.x, #tex0_tex1_tex2_tex3
mul r10, r1, r2

; -- Handle the else case by setting r11.w -------------------------------------

; If r0.x < #weight_normal_diffuse_specular.x the input attribute was < weight
slt r11.x, r0.x, #weight_normal_diffuse_specular.x

; If r2.w is 1 (attribute >= tex3) and r10.w != 1 (attribute != tex3) the
; input attribute was > tex3
slt r11.y, r10.w, r2.w

; Set r11.w = 1.0 if #test_attribute is < 0 or > ATTR_TEX3
add r11.w, r11.x, r11.y


; -- Apply the attributes as colors --------------------------------------------

; r3 = the unsupported color if #test_attribute was invalid.
mul r3, r11.w, #unsupported_color

; r3 = {iWeight, iWeight, 1, 1} if #test_attribute == ATTR_WEIGHT, else r3
mov r4.rg, iWeight.rr
mov r4.ba, #one_constant
mad r3, r4, r8.x, r3

; r3 = {iNormal.xyz, 1} if #test_attribute == ATTR_NORMAL, else passthrough
mov r4, iNormal
mov r4.a, #one_constant
mad r3, r4, r8.y, r3

; r3 = iDiffuse if #test_attribute == ATTR_DIFFUSE, else passthrough
mad r3, iDiffuse, r8.z, r3

; r3 = iSpecular if #test_attribute == ATTR_SPECULAR, else passthrough
mad r3, iSpecular, r8.w, r3

; r3 = {iFog, iFog, 1, 1} if #test_attribute == ATTR_FOG, else passthrough
mov r4.rg, iFog.rr
mov r4.ba, #one_constant
mad r3, r4, r9.x, r3

; r3 = {iPts, iPts, 1, 1} if #test_attribute == ATTR_POINT_SIZE, else passthrough
mov r4.rg, iPts.rr
mov r4.ba, #one_constant
mad r3, r4, r9.y, r3

; r3 = iBackDiffuse if #test_attribute == ATTR_BACK_DIFFUSE, else passthrough
mad r3, iBackDiffuse, r9.z, r3

; r3 = iBackSpecular if #test_attribute == ATTR_BACK_SPECULAR, else passthrough
mad r3, iBackSpecular, r9.w, r3

; r3 = iTex0 if #test_attribute == ATTR_TEX0, else passthrough
mad r3, iTex0, r10.x, r3

; r3 = iTex1 if #test_attribute == ATTR_TEX1, else passthrough
mad r3, iTex1, r10.y, r3

; r3 = iTex2 if #test_attribute == ATTR_TEX2, else passthrough
mad r3, iTex2, r10.z, r3

; r3 = iTex3 if #test_attribute == ATTR_TEX3, else passthrough
mad r3, iTex3, r10.w, r3

mov oPos, iPos
mov oDiffuse, r3
