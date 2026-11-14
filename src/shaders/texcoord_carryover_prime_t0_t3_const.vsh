; A vertex shader that passes through position and diffuse color and writes the
; constant in c[96] to oTex0 and oTex3 for every vertex. oTex1 and oTex2 are not
; written.
;
; Used to "prime" texture coordinate outputs 0 and 3 with a known value.

#prime_value vector 96

mov oPos, iPos
mov oDiffuse, iDiffuse
mov oTex0, #prime_value
mov oTex3, #prime_value
