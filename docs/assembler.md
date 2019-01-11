Construiremos um MONTADOR de DOIS PASSOS.

Partindo de um código fonte em linguagem simbólica com a seguinte estruturação:

**LABEL MNEMONIC OPERAND ;COMMENT**

O arquivo fonte é lido DUAS VEZES, constituindo dois passos:

PASSO1:
	Analiza linha por linha em busca de labels, listando-os. A cada linha analizada deve-se:
		*Associar labels a seus respectivos endereçcos
		*Se a linha contiver operandos referenciados por labels, deve-se verificar se o label ja foi listado, caso nao tenha sido
		 adiciona-o com endereço indefinido;
		*Atualizar o _ci apropriadamente para cada tipo de instrução.
PASSO2:
	O segundo passo é responsável por gerar 3 tipos de arquivos:
	* Arquivo de listagem: Apresenta o formato:
		ADDRES	OBJECT	LINE	SOURCE
	(OBJECT é o código de máquina hexadecimal com os endereços ja resolvidos

	* Arquivo de labels: apresenta o formato:
		LABEL	VALUE

	* Arquivo objeto: contem o codigo de maquina equivalente ao código montado

	Cada linha de código com mnemonicos deve gerar um obj (hexadecimal)

	Cada linha deve ser analisada da seguinte maneira:
		*Se for apenas uma linha de comentário ou de label: apenas lita-la
		*Se contem mnemonico: Obter op
		*Se contem operando: Se for label -> resolve e gera o obj. Se for endereo -> gera o obj
