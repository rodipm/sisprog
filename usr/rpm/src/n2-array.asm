INICIO	@	/0300 ;N2 - Calcula o quadrado de um número
	OS	/01	;Habilita trace mode
	IO	/01
	MM	N
	JZ	END
	LD	UM
	MM	ODD
LOOP	MM	RES
	LD	N
	-	UM
	MM	N
	JZ	END
	LD	ODD
	+	UM[1]
	MM	ODD
	LD	RES
	+	ODD
	MM	RES
	JP	LOOP
END	LD	RES
	IO	/00	;Desabilita trace mode
	CN	/00
	@	/0500	;Dados
N	K	/00	;N (a ser calculado N2)
UM	K	/01
DOIS	K	/02
ODD	K	/00
RES	K	/00
	#	INICIO
