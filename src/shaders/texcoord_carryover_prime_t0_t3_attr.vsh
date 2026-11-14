; A vertex shader that passes through position and diffuse color and writes the
; per-vertex texture coordinate 0 and 3 attributes to oTex0 and oTex3. oTex1 and
; oTex2 are not written.
;
; Used to "prime" texture coordinate outputs 0 and 3 with a distinct value per
; vertex.

mov oPos, iPos
mov oDiffuse, iDiffuse
mov oTex0, iTex0
mov oTex3, iTex3
