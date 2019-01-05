	@	/0001	;Endereço inicial do loader
INIT	IO	/02	;GET DATA - Device 2 (file)
	MM	IADDR	;Salva primeiro byte do endereço inicial do programa
	MM	START	;Salva novamente para termos o endereço ao final
	+	SUM	;Soma o byte para checksum
	MM	SUM
	IO	/02	;GET DATA
	MM	IADDR2	;Salva o segundo byte do endereço inicial do programa
	MM	START2
	+	SUM	;Soma o byte para checksum
	MM	SUM
	IO	/02	;GET DATA
	MM	SIZE	;Salva o tamanho do programa (em bytes)
	+	SUM	;Soma o byte para checksum
	MM	SUM
LOOP	IO	/02	;GET DATA
	CN	/02	;Ativa modo indireto
	MM	IADDR	;Guarda (indiretamente) o byte lido no endereço atual 
	+	SUM	;Soma o byte para checksum
	MM	SUM
	LD	IADDR2	;Carrega o endereço atual
	+	ONE	;Atualiza (soma 1)
	MM	IADDR2	;Salva
	LD	SIZE	;Carrega o tamanho atual
	-	ONE	;Atualiza (subtrai 1)
	MM	SIZE	;Salva
	JZ	CHECK	;Se SIZE = 0 pula para o checksum
	JP	LOOP	;Caso contrario, continua lendo
CHECK	IO	/02	;GET DATA
	+	SUM	;Soma o byte de checksum ao valor da soma atual
	MM	SUM
	IO	/05
	JZ	END	;Se SUM = 0 finaliza
	CN	/00	;Caso contraio HALT MACHINE
END	CN	/02	;Ativa modo indireto
	JP	START	;Salta para o inicio do programa carregado
IADDR	K	/00
IADDR2	K	/00
SIZE	K	/00
ONE	K	/01
START	K	/00
START2	K	/00
SUM	K	/00
	#	INIT
