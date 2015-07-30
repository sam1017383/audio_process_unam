	.def _test
	.sect  "ramfuncs"
	.global _inicio							;Constantes en Q12 3895.527491
	.global _fin

_test:



     MOVW AL,@_inicio
     ADD AL,@_fin
     MOVW @_fin,AL
     LRETR
