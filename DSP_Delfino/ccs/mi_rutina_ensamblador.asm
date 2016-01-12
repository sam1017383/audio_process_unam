
	.def _mi_rutina_ensamblador
	.sect  "ramfuncs"
	.global _sampling_window
	.global _WINDOW_LENGTH
	.global _filtered_value
	.global _current_sample


_mi_rutina_ensamblador:

     MOVW AL,@_current_sample
     ADD AL, #30
     MOVW @_filtered_value,AL
     LRETR
