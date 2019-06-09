			;N2 - Calcula o quadrado de um número
INICIO	@	/0100
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
	+	DOIS
	MM	ODD
	LD	RES
	+	ODD
	MM	RES
	JP	LOOP
END	LD	RES
	CN	/00
	@	/0500	;Dados
N	K	/00	;N (a ser calculado N2)
UM	K	/01
DOIS	K	/02
ODD	K	/00
RES	K	/00
	#	INICIO
