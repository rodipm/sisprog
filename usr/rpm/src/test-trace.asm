INICIO	@	/2100	;Programa simples para demonstrar o trace
	OS	/01	;Enable Trace
	+	NUMS[0]
	+	NUMS[1]
	+	NUMS[2]
	OS	/00	;Disable Trace
	CN	/00	;HALT
	@	/21AA	;Dados
NUMS	K	/00
NUMS1	K	/01
NUMS2	K	/02
	#	INICIO
