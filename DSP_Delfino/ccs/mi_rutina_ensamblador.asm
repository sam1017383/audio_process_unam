
	.def _mi_rutina_ensamblador
	.sect  "ramfuncs"
	.global _sampling_window
	.global _filter_coefs
	.global _WINDOW_LENGTH
	.global _filtered_value
	.global _newest_index
	.global _newest_inverted_index


_mi_rutina_ensamblador:
	; Esta rutina calcula el producto punto entre un buffer circular y un vector
	; Se utiliza para calcular el valor de señal filtrada entre un buffer circular
	; de muestras historicas y un vector de coeficientes de un filtro digital FIR
	;carga en apuntadores AR:
	; XAR7 --> dir inicio de vector de coeficientes de filtro
	; XAR3 --> dir inicio de vector de muestreo
	; XAR5 --> offset de dato mas reciente
    ; XAR2 --> dir vector de muestreo dato actual
    MOVL XAR7, #_filter_coefs
    MOVL XAR3, #_sampling_window
    MOVL XAR5, #_newest_index
    MOVL ACC, XAR3
    ADD ACC, *XAR5
    MOVL XAR2, ACC

	; El producto punto del buffer circular y con un vector
	; se dividira en dos partes por la estructura del buffer
    ; primera parte: filtro(0, newest) con samples(newest, WINDOW_LENGTH)
    ; XAR2 --> dir vector de muestreo con offset de dato actual
    ; XAR7 --> dir inicio de vector de coeficientes de filtro
    ZAPA
    RPT @_newest_inverted_index
  ||MAC P, *XAR2++, *XAR7++
	NOP
	; segunda parte: filtro(newest, WINDOW_LENGTH) con samples(0, newest)
	; XAR2 --> dir inicio de vector de muestreo sin offset
    ; XAR7 --> dir de vector de coeficientes de filtro con el incremento del ciclo pasado
	MOVL XAR2, #_sampling_window
	RPT @_newest_index
  ||MAC P, *XAR2++, *XAR7++
  	NOP
  	; Escala de valor acumulado
  	MOV T, #0x0
  	LSRL ACC, T
  	; Guarda el calculo en la variable de C filtered_value
	MOVL XAR5, ACC
	MOVL XAR3, #_filtered_value
	MOV *XAR3, AR5

    LRETR
