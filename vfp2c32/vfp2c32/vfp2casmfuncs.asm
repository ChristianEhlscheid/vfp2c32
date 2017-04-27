.586              ;Target processor.  Use instructions for Pentium class machines
.MODEL FLAT, STDCALL    ;Use the flat memory model. Use stdcall calling conventions
.STACK            ;Define a stack segment of 1KB (Not required for this example)
.DATA             ;Create a near data segment.  Local variables are declared after
                  ;this directive (Not required for this example)
.CODE             ;Indicates the start of a code segment.

LOWORD EQU [0]
HIWORD EQU [4]

Multiply_Int64 PROC USES ebx esi edi, ALO:DWORD, AHI:DWORD, BLO:DWORD, BHI:DWORD, OF:PTR

;       AHI, BHI : upper 32 bits of A and B
;       ALO, BLO : lower 32 bits of A and B
;
;step1	find sign of result
;step2	convert A into absolute
;step3	convert B into absolute
;step4	if both highwords are zero do mul only the low words
;step5   ALO * BLO
;       ALO * BHI
; +     BLO * AHI
; step6 fix sign of result
; ---------------------
	LOCAL result:QWORD
	LOCAL hiqword:QWORD
	
step1:						; move sign of result into esi register
    mov	eax, AHI			; move high word of A into eax
    mov	ecx, BHI			; move high word of B into ecx
    mov edx, eax			; store high word for later
    xor eax, ecx			; xor both to find sign
    mov esi, eax			; store sign into esi

step2:						; convert A into absolute
    mov	eax, edx			
    cmp eax, 0
    jns step3
    not ALO
    not	eax
	add ALO, 1
	adc eax, 0
	mov AHI, eax
	
step3:						; convert B into absolute
    cmp ecx, 0
    jns step4
	not BLO
	not ecx
	add BLO, 1
	adc ecx, 0
	mov BHI, ecx
	
; if both high words are zero we can do a sigle mul of both low words, else jump to step 5
step4:
    or	ecx, eax			; test for both hiwords zero.
    mov	ecx, BLO			; move low word of B into ecx
    jnz	step5				; both are zero, just multiply ALO and BLO

    mov	eax, ALO			; move low word of A into eax
    mul	ecx					; multiply both low words

   	test edx, 80000000h		; test for overflow
   	jnz	overflow
	jmp step6				; finished ...
	
; Multiply the L.O. word of Multiplier times Multiplicand:	
step5:
	mov eax, ALO
	mov ebx, eax			; save Multiplier val
	mul ecx					; multiply L.O. words
	mov edi, eax			; Save partial product
	mov ecx, edx			; move overflow into ecx

	mov eax, ebx			; move ALO to eax	
	mul BHI					; multiply ALO * BHI
	jc overflow				; check overflow
	add eax, ecx			; add carry from ALO * BLO
	jc overflow				; check overflow
	mov ebx, eax			; save partial result

; Multiply the H.O. word of Multiplier times Multiplicand:
	mov eax, AHI			; move AHI to eax
	mul BLO					; multiply AHI * BLO
	jc overflow				; check overflow
	add eax, ebx			; add partial result from ALO * BHI
	mov ebx, eax			; save partial result

	mov eax, AHI			; move AHI to eax
	mul BHI					; multiply AHI * BHI
	jc overflow				
	add eax, ebx
	jc overflow

	mov edx, eax
	mov eax, edi

step6:

; adjust sign of result
	cmp esi, 0				; compare sign
	jns short finish		; if positive return
	
;convert two complement positive to negative
	not eax
	not edx
	add eax, 1
	adc edx, 0

	test edx, 80000000h
	jz overflow

finish:
	mov ebx, of 
	mov byte ptr [ebx], 0
    ret

overflow:
	mov ebx, of
	mov byte ptr [ebx], 1
	ret
	
Multiply_Int64 ENDP 
END 

