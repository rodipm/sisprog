# Sistemas de Programação - PCS 3216 - 2019 #

Rodrigo Perrucci Macharelli - 9348877

--------------------

## Introdução ##

O projeto tem como objetivo implementar programaticamente um sistema computacional funcional que possibilite a programação e execução
de código em linguagem de máquina aboluta. Para isso, o sistema baseia-se em em uma **Máquina Virtual**, um **Interpretador** (interface de
linhas de comando - CLI) da linguagem de máquina absoluta (que será apresentada mais adiante) e um **Montador**, todos codificados em linguagem 
de alto nível. 

O primeiro será responsável por simular um sistema computacional simples, regido por uma linguagem de máquina absoluta, 
este programa será executado na máquina hospedeira, assim como o interpretador, que é responsavel pela comunicação entre o 
usuario (na máquina hospedeira) e a máquina virtual. O montador, assim como todos os outros programas básicos do sistema, pode ser acessado
pelo interpretador, e é responsável por gerar o código objeto a partir de um código em linguagem simbólica.

Além desses, o sistema ainda aprenseta outros  programas básicos, como um **Loader**, **Dumper**, e programas de teste, 
que foram codificados em linguagem simbólica.

## Máquina Virtual ##

### Descrição ###

A máquina virtual será responsável por executar os programas (código objeto) carregados em sua memória interna. O objetivo desse componente
do sistema de programação é o de simular um hardware programável.

Esta consiste em:
* 16 bancos de memória (0x0 a 0xF) cada um com 4k bytes (0x000 a 0xFFF), formando a memória principal da máquina virtual, na qual serão
  armazenadas as instruções de máquina (código objeto);
* 4 registradores, sendo eles:
	* _acc_: Acumulador com 8 bits (1 byte). Utilizado para operações aritiméticas;
	* _ci_: Contador de instruções com 16 bits (2 bytes). Responsável por manter o endereço correspondente a instrução atual;
	* _ri_: Registrador de instruções com 16 bits (2 bytes). Responsável por armazenar a instrução apontada pelo _ci_;
	* _cb_: Registrador de banco corrente com 4 bits. Responsável por armazenar qual é o banco de memória a ser referenciado
	  pelas instruções de referencia direta à memória.

### Funcionamento ###

O funcionamento básico da M.V. consiste em efetuar-se um procedimento de simulação de um processador, que deve seguir o seguinte algorítmo:

1. Baseado no endereço contido em _ci_, busca-se no banco de memória a instrução a ser executada;
2. A instrução corrente é armazenada em _ri_;
3. Em posse da instrução a ser executada, a M.V. deve decodificá-la, extraindo seu Código de Operação (CO) e operandos (OP), quando
   existirem;
4. Após decodificar a instrução, pode-se executar os procedimentos necessários com os operandos dados;
5. Atualiza-se o _ci_ e retoma-se o passo 1.

A implementação da M.V. foi feita em linguagem de alto nível (C++), e é orquestrada pelo **CLI**.

### Implementação ###
 
A definição da maquina virtual é a seguinte:
```cpp
class VM 
{
	// Banco de memórias
	uint8_t mem[banks][bank_size];

	// Registradores
	int8_t _acc; // acumulador
	uint8_t _cb; // banco corrente
	uint16_t _ri, _ci; // registrador de intruções e contador de intruções
	

	// Tabela de tamanhos das instruções
	const int instruction_sizes[0xd] = {2,2,2,1,2,2,2,2,2,2,2,1,1};
	
	// Variáveis de estado da maquina virtual e controle	
	bool running; // controle de execução da VM
	bool indirect; // modo indireto
	int step; // contador de passos
	update_ci_info *ci_info; // estrutura de dados para atuailização do contador de intruções
	bool DEBUG; // DEBUG flag
	uint16_t _initial; // endereço para debug
	bool trace; // flag trace mode
	
	// Io devices array
	// Podem ser referenciados 3 devices pela instrucao IO, ja que o primeiro referencia todos
	// o primeiro par é reservado para stdin e stdout.
	// os dois ultimos ficam livres para serem utilizados.
	// [0] = [cin, cout] \x1
	// [1] = [nullptr, nullptr] \x2
	// [2] = [nullptr, nullptr] \x3
	streambuf *io_devices[3][2];

	// Stream para read e write (inicializada com nullptr)
	iostream *stream;	
	
	// File stream for io_devices[2];
	fstream *file_stream;
	// Member Instruction Pointer para guardar a instrução a ser executada (desacoplando fetch e decode)
	void (VM::*execute)(void);
public:
	//...
```
Nota-se o atributo `cpp update_ci_info *ci_info` que tem seu tipo definido como:
```cpp
struct update_ci_info {
	bool		redirected;
	uint16_t 	addr;
	uint8_t		size; 

	update_ci_info() {
		redirected = false;
		addr = 0xdead;
		size = 0;
	}	
};
```
E é utilizada na função `update_ci()`, como será mostrado posteriormente.

A VM é instanciada segundo o seguinte construtor:
```cpp
VM() {
		// Inicialização do banco de memórias
		for (int i = 0; i < banks; i++)
			for (int j = 0; j < bank_size; j++)
				mem[i][j] = 0;	
	
		// Inicialização dos registradores
		_acc = 0;
		_cb  = 0;
		_ri = 0;
		_ci = 1;
		_initial = 0;

		// Demais inicializações
		running = false;
		indirect = false;
		step = 0;
		ci_info = new update_ci_info();
		trace = false;

		// Inicialização do device padrao (stdin e stdout)
		stream = new iostream(nullptr);
		stream->rdbuf(nullptr);
		io_devices[0][0] = cin.rdbuf();
		io_devices[0][1] = cout.rdbuf();
		for (int i = 1; i < 3; i++)
			for (int j = 0; j < 2; j++)
				io_devices[i][j] = nullptr;
	}
```

O funcionamento da MV segue a seguinte lógica de execução:

1. **Start**:
```cpp
// Inicializações do sistema
void start() {
	preloader();
	step = 0;
}
```
Executa a inicialização do sistema excecutando a funcao `preloader()` e zerando `step`.

O **Preloader**:
```cpp 
	// Preloader carregará o loader na memória
	void preloader() {
		// Carrega o arquivo loader0.bin
		fstream file("./bin/loader0.bin", ios_base::in | ios_base::binary);
		if (DEBUG) cout << endl << "#############PRELOADER#############" << endl;
		
		// Le o arquivo em str
		stringstream str;
		char c;
		while (file.get(c)) {
			unsigned int tmp =  (unsigned int)c;
			str << hex << setfill('0') << setw(2) << (tmp & 0xff);
		}

		if (DEBUG) cout << str.str() << endl;
		
		// Inicio do carregamento do loader na memória
		int addr = 1;	
		
		// Carrega a partir do sexto byte (primeiro byte de dados)
		// Até o fim dos dados (sem checksum)
		for (int i = 6; i < str.str().length() - 2; i+=2) {
			char s[] = {str.str()[i], str.str()[i+1]};
			stringstream ss;
			ss << hex << s;
			unsigned short int tmp;
			ss >> tmp;
			mem[0][addr++] = tmp;
		}

		file.close();
		if (DEBUG) cout << endl << "#############END-PRELOADER#############" << endl;

	}
```
é responsável por carregar o programa `loader.asm` (escrito em linguagem simbólica) a partir do endereço /0001 de memória.

Com este carregado em memória, pode-se utilizar a própria VM para carregar futuros programas em memória, bastando obter os dados necessários
e executar a VM a partir do endereço /0001. Este processo será visto com mais detalhes a seguir.

2. **Load:**
```cpp
// Loader carrega arquivo requerido recebendo o nome do arquivo (sem nenhuma numeracao ou extensao)
// e numero de arquivos a ser carregados
void load(string file, int no, string client = "") {
	for (int i = 0; i < no; i++) {
		// Prepara o nome do arquivo a ser utilizado com io_device
		stringstream name;
		if (client != "")
			name << "./usr/" << client;
		else
			name << ".";
		name << "/bin/" + file << i << ".bin";
		file_stream = new fstream(name.str(), ios_base::in | ios_base::out | ios_base::binary);
		// Efetua o link do buffer do arquivo com a lista de devices acessíveis via linguagem assembly
		io_devices[1][0] = file_stream->rdbuf();
		io_devices[1][1] = file_stream->rdbuf();
		
		// Executa o Loader (Endereço /0001)
		run(1);
		file_stream->close();
		delete file_stream;
	}
}	
```
Essa função atua como um encapsulador para o funcionamento do loader carregado na memória da máquina virtual. Sua principal função é
associar os buffers dos arquivos corretos aos devices de input e output. A utilização dos devices será vista em detalhes
na apresentação da linguagem de máquina e instruções utilizadas na implementação do projeto.

Nota-se nesse trecho de código a primeira referencia a `cliente` que é uma variável utilizada pelo CLI que deve ser repassada para os
outros módulos (Assembler e VM), de modo a caracterizar o usuário associado a sessão atual do CLI. Maiores explicações sobre a estrutura
de dados e funcionamento do CLI serão feitas na seção sobre o Interpretador.

3. **Run:**
```cpp
// Inicia a maquina
void run(int initial = 0, bool deb = false) {
	// Inicia os valores dos contadores de Instrução e de Banco Corrente
	_initial = initial;
	_ci = initial & 0x0fff;
	_cb = initial >> 12;

	running = true; // inicia execução da maquina
	DEBUG = deb; // debug mode

	// Loop Principal da MV
	while (running) {
		// Executa o Fetch (Busca) da instrução atual	
		int instructionSize = this->fetch();
		
		// Decodifica a instrução atual
		this->decode();

		// Executa a instrução atual com o tratamento adequado obtido na decodificação
		(this->*execute)();

		// Chamada da função de trace que mostra o estado da VM a cada passo
		do_trace(instructionSize);

		//atualiza o _ci
		update_ci(instructionSize);

		step++;
	}
}	
```
Essa função é a principal responsável pelo funcionamento da máquina virtual, executando as etapas necessárias para tal.
Tais etapas são as seguintes:

#### **Fetch:** ####

```cpp
// Busca a instrução a ser executada baseada em _ci e a armazena em _ri
int fetch() {
	// Verifica o CO da instrução para saber qual o seu tamanho
	// Utilizando a tabela de tamanho de instruções
	uint8_t instruction_size = instruction_sizes[mem[_cb][_ci] >> 4];

	// Obtem o primeiro byte da instrução
	_ri  = mem[_cb][_ci];
	_ri = _ri << 8;
	// Caso seja uma instrução de 2 bytes, concatena o segundo byte a _ri
	if (instruction_size == 2 || indirect) {
		_ri |= (uint16_t)mem[_cb][_ci + 1] & 0x00ff;
	}
	
	return instruction_size;
}
```
É responsável por obter a instrução apontada por _ci_ na memória e guardá-la em _ri_.

#### **Decode:** ####
```cpp
// Interpreta a instrução e encaminha para a execução correta
uint16_t decode() {
	// Obtem o CO da instrução
	uint16_t CO = _ri >> 4*3;

	// garante que o modo indireto só se mantem em caso de instrução de acesso à memóra
	if ((CO == 0x3 || CO > 0xa) && indirect)
		indirect = false;

	switch(CO) {
		case 0x0: //JP
			execute = &VM::JP;
			break;
		case 0x1: //JZ
			execute = &VM::JZ;
			break;
		case 0x2: //JN
			execute = &VM::JN;
			break;
		case 0x3: //CN
			execute = &VM::CN;
			break;
		case 0x4: // +
			execute = &VM::plus;
			break;
		case 0x5: // -
			execute = &VM::minus;
			break;
		case 0x6: // *
			execute = &VM::multiply;
			break;
		case 0x7: // /
			execute = &VM::divide;
			break;
		case 0x8: // LD
			execute = &VM::LD;
			break;
		case 0x9: // MM
			execute = &VM::MM;
			break;
		case 0xa: //SC
			execute = &VM::SC;
			break;
		case 0xb: // OS
			execute = &VM::OS;
			break;
		case 0xc: // IO
			execute = &VM::IO;
			break;
		default:
			cout << "Instrução inválida" << endl;
			cout << "_ri: " << hex << _ri << endl;
			break;
	}
	return -1;
}
```
Executa a decodificação da instrução com base no Código de Operação (CO) da instrução. Dessa forma utiliza-se um case para definir
o ponteiro da função que deve ser executada na parte de Execução descrita a seguir.

#### Execute: ####
```cpp
(this->*execute)();
```
É responsável por executar uma das seguintes possíveis rotinas de tratamento para cada uma das instruções de máquina disponíveis na linguagem assembly adotada. Cada instrução será detalhada em conjunto com sua rotina de tratamento. No próximo capítulo, sobre a linguagem assembly adotada essas instruções serão mais detalhadas.

* **Jump**
```cpp
// Instrução JUMP UNCONDITIONAL 0x0XXX
void JP() {
	if (!indirect) { // Verificação de modo indireto de endereçamento
		ci_info->addr	= _cb << 4; // obtem o valor de _cb
		ci_info->addr	= ci_info->addr << 4*3; // shift de 3 bytes a esquerda para _cb ser o primeiro byte
		ci_info->addr	|= _ri & 0x0fff; // AND logico com o valor dos 3 ultimos bytes da instução

	}
	else {
		uint16_t ptr  = getIndirectPtr(); // obtem o valor do endereço referenciado indiretamente
		_cb = (uint8_t)(ptr >> 4*3); // obtém o novo _cb
		indirect = false; // reseta o modo indireto
		ci_info->addr = ((_cb << 4) << 4*3 ) | (ptr & 0x0fff); // concatena _cb e o endereço
	}
	ci_info->redirected = true; // flag de redirecionamento
}

// retorna o endereço contido em uma celula de memória referenciada por acesso indireto à mem.
uint16_t getIndirectPtr() {
	uint16_t OP = _ri & 0x0fff; // obtém o endereço de memória referenciado
	uint16_t ptr  = ((uint16_t)mem[_cb][OP] << 8) & 0xff00 | ((uint16_t)mem[_cb][OP + 1] & 0x00ff); // concatena os dados da memória
	return ptr;
}	
```
Tratamento da instrução de desvio incondicional, apresenta como OPcode o valor 0x0. Pode ser utilizado com o mnemonico JP /xxx, fazendo com que o *_ci* receba o valor /yxxx, sendo y o valor atual de *_cb*.

* **Jump If Zero**
```cpp
// Instrução JUMP IF ZERO 0x1XXX
void JZ() {
	if (_acc == 0)
		this->JP();
}
```

A instrução Jump if Zero apresenta OPcod 0x1 e verifica se o acumulador tem o valor 0 e, caso afirmativo, efetua o tratamento de JP para o endereço desejado. Sua forma mnemonica é: JZ /xxx.

* **Jump If Negative**
```cpp
// Instrução JUMP IF NEGATIVE 0x2XXX
void JN() {
	if (_acc < 0)
		this->JP();
}
```
Similar a instrução JZ, a instrução Jump if Negative, OPcode 0x2, verifica se o acumulador apresenta valores negativos e, caso afirmativo, aciona a rotina de JP. Sua representação mnemonica é: JN /xxx.

* **Controle**
```cpp
// Instrução CONTROLE 0x3X
void CN() {
	switch((_ri & 0x0f00) >> 8) {
		case 0x0: // HLT
			if(DEBUG) cout << "Machine is halted" << endl;
			running = false;
			break;
		case 0x1: // RI
			//cout << "Return from interrupt" << endl;
			break;
		case 0x2: // IN
			//cout << "Indirect mode ON" << endl;
			indirect = true;
			break;
		case 0x3: // NOP;
			//cout << "NOP" << endl;
			break;
		default:
			cout << "Instrução invalida!" << endl;
			cout << "_ri: " << hex << _ri << endl;
			exit(1);
			break;
	}	
}
```
As instruções de controle, OPcode 0x3, podem ser utilizadas para acionar modo de endereçamento indireto, parar (halt) a máquina, retorno de interrupção (não implementado, apenas utilizado para possível implementação) e representação de NOP (no operation). Sua forma mnemonica é: CN /xx, no qual /xx representa um dos três códigos a seguir: [/00] HALT MACHINE, [/01] RETURN FROM INTERRUPT, [/02] INDIRECT MODE, [/03] NOP.

O **Modo Indireto** possibilita que se referencie um endereço de memória dentro de um mesmo banco de memória (como em todas as funções de desvio acima) que contém um segundo endereço de memória de 4 bytes (incluindo banco de memória), no qual será efetuada a operação em modo indireto.

* **Soma**
```cpp
// Instrução PLUS 0x4XXX
void plus() {
	if (!indirect) {
		_acc += (int8_t)mem[_cb][_ri & 0x0fff]; // se modo indireto efetua a soma do valor atual de _acc com o valor da instrução
	}
	else {
		uint16_t ptr = getIndirectPtr(); // obtém o valor referenciado indiretamente
			_acc += (int8_t)mem[ptr >> 4*3][ptr & 0x0fff]; // efetua a soma
		indirect = false;
	}	
}
```
Insrtução de soma, OPcode 0x4, cuja representação mnemonica é: + /xxx. Atualiza o valor de *_acc*.

* **Subtração**
```cpp
// Instrução MINUS 0x5XXX
void minus() {
	if (!indirect) {
		_acc -= (int8_t)mem[_cb][_ri & 0x0fff];
	}	
	else {
		uint16_t ptr = getIndirectPtr();
		_acc -= (int8_t)mem[ptr >> 4*3][ptr & 0x0fff];
		indirect = false;
	}	
}
```
Instrução de subtração, OPcode 0x5, cuja representação mnemonica é: - /xxx. Atualiza valor de *_acc*.

* **Multiplicação**
```cpp
// Instrução MULTIPLY 0x6XXX
void multiply() {
	if (!indirect) {
		_acc *= (int8_t)mem[_cb][_ri & 0x0fff];
	}	
	else {
		uint16_t ptr = getIndirectPtr();
		_acc *= (int8_t)mem[ptr >> 4*3][ptr & 0x0fff];
		indirect = false;
	}	
}
```
Instrução de multiplicação, OPcode 0x6, cuja representação mnemonica é: * /xxx. Atualiza valor de *_acc*.

* **Divisão**
```cpp
// Instrução DIVIDE 0x7XXX
void divide() {
	if (!indirect) {
		_acc /= (int8_t)mem[_cb][_ri & 0x0fff];
	}	
	else {
		uint16_t ptr = getIndirectPtr();
		_acc /= (int8_t)mem[ptr >> 4*3][ptr & 0x0fff];
		indirect = false;
	}	
}
```
Instrução de divisão, OPcode 0x7, cuja representação mnemonica é: / /xxx. Atualiza valor de *_acc*.

* **Load (Carrega da Memória)**
```cpp
// Instrução LOAD FROM MEMORY 0x8XXX
void LD() {
	if (!indirect) {
		_acc = (int8_t)mem[_cb][_ri & 0x0fff]; // carrega em _acc o valor referenciado na memória
	}	
	else {
		uint16_t ptr = getIndirectPtr(); // obtém o valor indiretamente referenciado
		_acc = (int8_t)mem[ptr >> 4*3][ptr & 0x0fff];
		indirect = false;
	}	
}
```
A instrução Load, OPcode 0x8, efetua o carregamento de um byte da memória e o carrega no acumulador. Sua representação mnemonica é: LD /xxx.

Pode ser utilizado posteriormente a uma instrução CN /02 para efetuar a leitura de um endereço em modo indireto.

* **Move (Move Para a Memória)**
```cpp
// Instrução MOVE TO MEMORY 0x9XXX
void MM() {

	if (!indirect) {
		mem[_cb][_ri & 0x0fff] = (uint16_t)_acc & 0x00ff;
	}	
	else {
		uint16_t ptr = getIndirectPtr();
		mem[ptr >> 4*3][ptr & 0x0fff] = (uint16_t)_acc & 0x00ff;
		indirect = false;
	}	
}
```
A instrução MOVE, OPcode 0x9, efetua o salvamento dos dois bytes contidos em *_acc* em dois  bytes consecutivos de memória contidos na instrução ou de modo indireto. Sua representação mnemonica é: MM /xxx.

* **Chamada de Subrotina**
```cpp
// Instrução SUBROTINE CALL 0xAXXX
void SC() {
	// A instrução deve guardar o endereço de retorno em \yxxx (com base em _cb) e executar a próxima instrução (\yxxx + 0x2)
	uint16_t ret_addr = _ci;
	mem[_cb][_ri & 0x0fff] = (uint8_t)ret_addr;
	mem[_cb][(_ri & 0x0fff) + 1] = (uint8_t)(ret_addr);
	ci_info->addr = (_cb << 4) | (_ri & 0x0fff);  
	ci_info->redirected = true;
	
}
```
A instrução de subroutine call, OPcode 0xA, permite a execução de subrotinas guardadas em memória, possibilitando a reutilização de código e trabalhando com umum sistema simples de tratamento do endereço de retorno, apenas o inserindo em um endereço de memória. Esta implementação, como está, não possibilita a chamada de rotinas de maneira recursiva, podendo ser implementada com um sistema de armazenamento em pilha, não contemplado neste projeto. Sua representação mnemonica é: SC /xxx.

* **Chamada de Sistema Operacional**
```cpp
// Instrução de chamada do sistema operacional 0xBX
void OS() {
	switch((_ri & 0x0f00) >> 8) {
		case 0x0: // DISABLE TRACE
			this->trace = false;
			break;
		case 0x1: // ENABLE TRACE
			this->trace = true;
			break;
		default:
			cout << "Instrução invalida!" << endl;
			cout << "_ri: " << hex << _ri << endl;
			exit(1);
			break;
	}
}
```
A instrução de Chamada de Sistema Operacional, OPcode 0xB, pode ser utilizada para habilitar o modo trace de execução, que efetua o log da região de memória do programa e dados de execução. Sua representação mnemonica é: OS /xx. Sendo xx um código de operação, sendo [/00] Disable Trace Mode e [/01] Enable Trace Mode.

* **Input Output**
```cpp
// Instrução INPUT OUTPUT 0xCX
void IO() {
	// Operando (4 ultimos bits)
	uint16_t OP = ((_ri & 0x0f00) >> 8);
	// Device a ser operado
	uint16_t DEV = OP & 0x3; // 0x3 = 0b0011 -> mascara para os dois últimos bits apenas

	// Para transformar int em char *, primeiramente transformamos em string e depois em c_str (char *).
	stringstream str;
	str << hex << setfill('0') << setw(2) << (uint16_t)_acc;

	// Processamento do OP para verificar que tipo de instrução de IO deve ser executada
	// verifica os dois bits mais significativos dos ultimos 4 bits da instruçao
	// PUT DATA
	if (OP & 0x4) { // 0b0100
		if (DEV == 0x0) {
			for (int i = 0; i < 3; i++) {
				if (io_devices[i][1] != nullptr) {
					stream->rdbuf(io_devices[i][1]);
					stream->write(str.str().c_str(), 2);
				}
			}
				
		}
		else{
			if (io_devices[DEV-1][1] != nullptr) {
				stream->rdbuf(io_devices[DEV-1][1]);
				char value = (char)_acc;
				stream->write(&value, 1);
				cout << "########WRITING " << value << endl;
			}
		}
		stream->rdbuf(nullptr);
	}

	// ENABLE INTERRUPT
	else if (OP & 0x8) { //0b1000
		cout << "ENABLE INTERRUPT" << endl;
	}

	// DISABLE INTERRUPT
	else if (OP & 0xc) { //0b11000
		cout << "DISABLE INTERRUPT" << endl;
	}

	// GET DATA
	else { //0b0000
		if (DEV != 0) {
			if (io_devices[DEV-1][0] != nullptr) {
				if (stream->rdbuf() == nullptr) {
					stream->rdbuf(io_devices[DEV-1][0]);
				}
				char buff[3];
				if (DEV == 1) {
					cout << "Input: ";
					stream->read(buff, 3);
					for (int i = 0; i < 2; i++) {
						if (buff[i] >= 48 && buff[i] < 97)
							buff[i] -= 48;
						else if (buff[i] >= 97)
							buff[i] -= 97 - 10;
					}
					buff[0] = (buff[0] & 0x0f) << 4 | (buff[1] & 0x0f);
				}
				else
					stream->read(buff, 1);
				stream->rdbuf(nullptr);
				_acc = (int8_t)buff[0];
			}
			else {
				cout << "TRYING TO GET DATA FROM UNKNOWN FILE" << endl;
				exit(1);
			}
		}
	}
}
```
Instrução e Input Output, OPcode 0xC, controla as operações de input e output, de forma que pode-se escrever (PUT DATA) e ler (GET DATA) das possiveis fontes. Existem 4 possíveis devices fonte: [1] - Cin / Cout, [2] e [3] - Disponíveis para o programador. A utilização dessa instução é feita seguindo o seguinte formato mnemonico: IO	/od, em que 'o' representa a operação a ser realizada: [0] - Get Data, [1] - Put Data,  [2] - Enable Interrupt, [3] - Disable Interrupt, sendo que as duas últimas não foram implementdas, como citado anteriormente. E 'd' representa um dos possíveis devices.
 
Para finalizar as funcionalidade da maquina virtual, segue o código que encapsula o dumper, escrito em linguagem assembly (mostrado adiante).
```cpp
// Dumper guarda parte da memória em um arquivo escolhido
void dump(string file, string client) {
	stringstream name;
	name << "./usr/" << client << "/bin/" << file << ".dmp.bin";
	file_stream = new fstream(name.str(), ios_base::out | ios_base::binary);
	io_devices[1][0] = file_stream->rdbuf();
	io_devices[1][1] = file_stream->rdbuf();
	if(DEBUG) cout << "#####RUNNING DUMPER#####" << endl;
	run(0x50, true);
	file_stream->close();
	delete file_stream;
}
```
Essa função efetua os links corretos dos buffers de leitura e escrita para serem utilizados pela VM na interpretação do dumper e inicia a execução da VM a partir do endereço 0x50 de memória, que contem o dumper.

## LINGUAGEM SIBÓLICA ##

Para a simulação de um sistema de programação, deve-se definir em que linguagem esse sistema pode ser programado. Define-se neste capítulo a linguágem simbólica utilizada para tal fim.
Trata-se da arquitetura apresentada em sala de aula (Aula 09), que embora seja uma arquitetura simples, podendo simular eventos de loop, decisão, operações aritméticas, de entrada e saída e de controle.

### INSTRUÇÕES DE DESVIO ###

1. Jump Unconditional | JP /xxx | /0xxx
Efetua desvio incondicional para endereços (/xxx)  dentro do mesmo banco de memória (/y).
> ci = /yxxx

2. Jump if Zero | JZ /xxx | /1xxx
Efetua desvio para endereços (/xxx) dentro do mesmo banco de memória (/y) se acumulador igual a zero.
> if acc == 0 then ci = /yxxx; else ci = ci + instructionSize

3. Jump if Negative | JN /xxx | /2xxx
Efetua desvio para endereços (/xxx) dentro do mesmo banco de memória (/y) se acumulador menor que zero.
> if acc < 0 then ci = /yxxx; else ci = ci + instructionSize

### INSTRUÇÕES DE CONTROLE ###

São acessadas como operações executadas pela instrução Control (CN).

1. Halt Machine | CN /00 | /30
Para a execução da máquina.

2. Return From Interrupt | CN /01 | /31
Operação não implementada neste projeto.

3. Indirect Mode | CN /02 | /32
Aciona o modo indireto de endereçamento de memória. Neste modo pode-se acessar regiões de memóra de bancos diferentes.

4. No Operation | CN /03 | /33
Instrução nula.

### INSTRUÇÕES ARITIMÉTICAS ###

1. Soma | + /xxx | /4xxxx

> acc = acc + /xxx

2. Subtração | - /xxx | /5xxx

> acc = acc + /xxx

3. Multiplicação | * /xxx | /6xxx

> acc = acc + /xxx

4. Divisão | / /xxx | /7xxx

> acc = acc / /xxx

### INSTRUÇÕES DE ACESSO A MEMÓRIA ###

1. Load From Memory | LD /xxx | /8xxx

Efetua a leitura de um endereço de memória e guarda o byte no acumulador.
> acc = memory[/xxx]

2. Move to Memory | MM /xxx | /9xxx

Efetua a escrita em memória com os dois bytes contidos em /yxxx e /yxxx + /01.
> acc = memory[/yxxx] | memory[/yxxx + 1]

### INSTRUÇÃO DE CHAMADA DE SUBROTINA ###

1. Subroutine Call | SC /xxx | /Axxx

Efetua a chamada de subrotina alocada no endereço /yxxx.
A estrutura adotada para se trabalhar com subrotinas segue o seguinte formato:
> memory[/yxxx] 		= ENDEREÇO DE RETORNO
memory[/yxxx + 2]	= PRIMEIRA INSTRUÇÃO DA SUBROTINA	  
