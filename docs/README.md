# Sistemas de Programação - PCS 3216 - 2019 #

Rodrigo Perrucci Macharelli - 9348877

--------------------

## **Intro** ##

O projeto tem como objetivo implementar programaticamente um sistema computacional funcional que possibilite a programação e execução
de código em linguagem de máquina aboluta. Para isso, o sistema baseia-se em em uma Máquina Virtual, um interpretador (interface de linhas
de comando) da linguagem de máquina absoluta (que será apresentada mais adiante) e um montador, todos codificados em linguagem de alto nível. 

O primeiro será responsável por simular um sistema computacional simples, regido por uma linguagem de máquina absoluta (a ser descrita
posteriormente), este programa será executado na máquina hospedeira, assim como o interpretador, que é responsavel pela comunicação entre o 
usuario (na máquina hospedeira) com a máquina virtual. O montador, assim como todos os outros programas básicos do sistema, pode ser acessado
pelo interpretador, e é responsável por gerar o código objeto a partir de um código em linguagem simbólica.

Além desses, o sistema ainda aprenseta outros  programas básicos, como um Loader e um Dumper, que foram codificados em linguagem 
simbólica(asm).

## **Máquina Virtual: Conceito da implementação** ##

A máquina virtual será responsável por executar os programas (código objeto) carregados em sua memória interna. O objetivo desse componente
do sistema de programação é o de simular um hardware programável.

Esta consiste em:
* 16 bancos de memória (0x0 a 0xF) cada um com 4k bytes (0x000 a 0xFFF), formando a memória principal da máquina virtual, na qual serão
  armazenadas as instruções de máquina (código objeto);
* 4 registradores, sendo eles:
	* _acc_: Acumulador com 8 bits (1 byte). Utilizado para operações aritiméticas;
	* _ci_: Contador de instruções com 16 bits (2 bytes). Responsável por manter o endereço correspondente a instrução atual;
	* _ri_: Registrador de instruções com 16 bits (2 bytes). Responsável por armazenar a isntrução apontada pelo _ci_;
	* _cb_: Registrador de banco corrente com 4 bits. Responsável por armazenar qual é o banco de memória a ser referenciado
	  pelas instruções de referencia direta à memória.

O funcionamento básico da M.V. consiste em efetuar-se um procedimento de simulação de um processador, que deve seguir o seguinte algorítmo:

1. Baseado no endereço contido em _ci_, busca-se no banco de memória a instrução a ser executada;
2. A instrução corrente é armazenada em _ri_;
3. Em posse da instrução a ser executada, a M.V. deve decodificá-la, extraindo seu Código de Operação (CO) e operandos (OP), quando
   existirem;
4. Após decodificar a instrução, pode-se executar os procedimentos necessários com os operandos dados;
5. Atualiza-se o _ci_ e retoma-se o passo 1.

A implementação da M.V. foi feita em linguagem de alto nível (C), e 

