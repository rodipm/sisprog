	@	/0050	;Endereço inicial do dumper 
INIT	IO	/01	;GET DATA - Device 1 (stdin) - Primeiro Byte
	MM	IADDR	;Salva primeiro byte do endereço inicial do programa
	IO	/06	;PUT DATA - Device 2 (file)
	+	SUM	;Soma o byte para checksum
	MM	SUM
	IO	/01	;GET DATA - Device 1 (stdin) - Segundo Byte
	MM	IADDR2	;Salva o segundo byte do endereço inicial do programa
	IO	/06	;PUT DATA - Device 2 (file)
	+	SUM	;Soma o byte para checksum
	MM	SUM
	IO	/01	;GET DATA - Device 1 (stdin) - Tamanho do programa
	MM	SIZE	;Salva o tamanho do programa (em bytes)
	IO	/06	;PUT DATA - Device 2 (file)
	+	SUM	;Soma o byte para checksum
	MM	SUM
LOOP	CN	/02	;Ativa modo indireto
	LD	IADDR	;Carrega (indiretamente) o byte lido do endereço atual 
	IO	/06	;PUT DATA - Device 2 (file)
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
CHECK	LD	COMP	;Carrega FF no acumulador
	-	SUM	;Efetua FF - SUM
	+	ONE	;Soma um
	IO	/06	;PUT DATA - Device 2 (file)
	CN	/00	;Halt Machine
IADDR	K	/00
IADDR2	K	/00
SIZE	K	/00
ONE	K	/01
COMP	K	/FF
START	K	/00
START2	K	/00
SUM	K	/00
	#	INIT
