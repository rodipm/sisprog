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

### INSTRUÇÃO DE CONTROLE ###

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
> memory[/yxxx] = ENDEREÇO DE RETORNO

> memory[/yxxx + 2] = PRIMEIRA INSTRUÇÃO DA SUBROTINA	  

Assim, o endereço de retorno deve ser armazenado nos dois bytes imediatamente anteriores à primeira instrução da subrotina. O endereço do primeiro byte do endereço de retorno (/yxxx) passa a ser o endereço chamado pela instrução SC, como mostrado em seu exemplo acima.

### INSTRUÇÃO DE SISTEMA OPERACIONAL ###

Para este projeto não há um sistema operacional propriamente dito, sendo o CLI responsável por algumas funções cabíveis ao sistema operacional. Tendo isso em vista, foram adotadas duas funcionalidades para a instrução de OS.

1. Disable Trace | OS /00 | /B0

Desabilita a funcionalidade de TRACE, que exibe, a cada instrução executada, um mapa da memória atualmente ocupada pelo código assim como dados de registradores, estado da máquina e outras informações.

2. Enable Trace | OS /01 | /B1

Habilita a funcionalidade de TRACE.

### INSTRUÇÃO DE ENTRADA E SAÍDA ###

Essa instrução permite acessar um dos 3 pares de buffer (entrada / saída ) disponíveis para o programador. Esses pares são tratados como dispositivos, que podem ser acessados para as operações de leitura e escrita.

Os devices acessíveis são:
* /00 - Escrita em todos os devices.
* /01 - Escrita / Leitura no dispositivo padrao de entrada e saída da máquina hospedeira.
* /02 - Escrita / Leitura em dispositivos definidos pelo programador.
* /03 - Escrita / Leitura em dispositivos definidos pelo programador.

As operações possíveis são:

1. Get Data | IO /00 - /03 | /C0 - /C3

Carrega um byte do dispositivo de entrada no acumulador.
> acc = device[/x]

2. Put Data | IO /04 - /07 | /C4 - /C7

Carrega dois bytes do acumulador no dispositivo de saída.
> device[/x] = acc

### PSEUDO INSTRUÇÕES ###

São instruções que não são direcionadas à execução da máquina virtual, mas sim para coordenar o funcionamento do Assembler.

1. Origin | @ /yxxx

Define a endereço de memória origem para a carga do programa em memória.

2. Array | $ /xxx 

Define alocação de memória para ser utilizado como vetor.

3. Constant | # /xx

Aloca no endereço de memória em que se encontra o valor /xx.

### Detalhes de Implementação ###

Embora não conste na linguagem apresentada em sala de aula, adaptou-se uma notação de vetor, utilizando-se de colchetes para envolver o índice desejado.

Segue um exemplo de elaboração de código na linguagem assembly adotada:

```
INICIO	@	/0100 ;N2 - Calcula o quadrado de um número
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

```

O programa acima calcula o quadrado de um valor inserido pelo usuário em tempo de execução do programa. Observando o código fonte, pode-se perceber algumas regras sobre a escrita nessa linguagem.
O formato padrão de uma linha de código segue o seguinte esquema:
> LABEL INSTRUÇÃO OPERANDO ;COMENTÁRIO

No qual cada um dos campos deve ser inserido sempre em sua respectiva coluna, cada uma separada por um TAB. Percebe-se a utilização das pseudo instruções para delimitar inicio e fim do código, assim como a definição de constantes e utilização de diversas instruções. Exemplos do funcionamento desse programa serão dados mais adiante.

## ASSEMBLER ##

### ESPECIFICAÇÃO ###

Construi-se um MONTADOR de DOIS PASSOS.

Partindo de um código fonte em linguagem simbólica com a seguinte estruturação:

> LABEL MNEMONIC OPERAND ;COMMENT

O arquivo fonte é lido DUAS VEZES, constituindo dois passos:

### DESCRIÇÃO DOS PASSOS DE MONTAGEM ###

* PASSO 1:
Analiza linha por linha em busca de labels, listando-os. A cada linha analizada deve-se:

	* Associar labels a seus respectivos endereçcos
	* Se a linha contiver operandos referenciados por labels, deve-se verificar se o label ja foi listado, caso nao tenha sido
	 adiciona-o com endereço indefinido;
	* Atualizar o _ci apropriadamente para cada tipo de instrução.

* PASSO 2:
O segundo passo é responsável por gerar 3 tipos de informações:

	* Listagem: Apresenta o formato:
	> ADDRES OBJECT LINE SOURCE

	(OBJECT é o código de máquina hexadecimal com os endereços já resolvidos)

	* Labels abels: apresenta o formato:
	> LABEL VALUE

	* Arquivo objeto: contem o codigo de maquina equivalente ao código montado

### TRATAMENTO DO CÓDIGO FONTE ###
Cada linha de código com mnemonicos deve gerar um obj (hexadecimal).

Cada linha deve ser analisada da seguinte maneira:

* Se for apenas uma linha de comentário ou de label: apenas lista-la
* Se contem mnemonico: Obter op
* Se contem operando: Se for label -> resolve e gera o obj. Se for endereo -> gera o obj

### IMPLEMENTAÇÃO ###

Abaixo encontra-se o código comentado da parte central de lógica do montador:
```cpp
assembled Assemble(string file, string client = "") {

	file_name = file;
	file_number = 0;
	_ci = 0;
	initial_addr = 0;
	block_size = 0;
	
	// Processa todas as linhas do arquivo
	vector<file_line> lines = processFile(file, client);

	// As linhas pós processadas serão armazenadas em um vector
	vector<string> list;		
	
	//tabela de simbolos
	map<string, string> simbols_list;
	

	// Codigos objeto gerados
	vector<obj> obj_list;

	// Trata-se de um assembler de dois passos, logo teremos de iterar duas vezes
	for (int step = 1; step < 3; step++) {
		_ci = 0;
		// Analizamos cada uma das linhas obtidas no processamento do arquivo
		for (int i = 0; i < lines.size(); i++) {

			if (lines[i].mnemonic == "") { // Comment or blank line (will be ignored)
				if ((step == 2) && (lines[i].comment != "")) { // Just a comment line
					do_list(list, lines[i].comment, i); // List comment line if step = 2
					continue;
				}
				continue;
			} 

			// Analise do label
			if (step == 1) {
				if (lines[i].label != "") { // Has label
					if (simbols_list.find(lines[i].label) == simbols_list.end()) { // Create new simbol 
						stringstream str;
						str << "/" << hex << setfill('0') << setw(4) << _ci;
						simbols_list[lines[i].label] = str.str();
					} else if (simbols_list[lines[i].label] == "undefined") { // Simbol undefined
						stringstream str;
						str << "/" << hex << setfill('0') << setw(4) << _ci;
						simbols_list[lines[i].label] = str.str();
					} else {
						cout << "ASSEMBLER ERROR: Label redefinition at line " << i + 1 << endl;
						exit(1);
					}	
				}	
			}

			// Analise do mnemonico

			// Tenta verificar qual o mnemonico encontrado no conjunto de instruções normais
			int op, size, arg;
			op = -1;
			size = 0;
			for (int j = 0; j < 0xd; j++) {
				if (mnemonic_table[j].name.find(lines[i].mnemonic) == 0) {
					op = mnemonic_table[j].op;
					size = mnemonic_table[j].size;
				}	
			}
			
			// Se nao foi encontrado, pode ser uma pseudoinstrução
			if (op == -1) {
				if (lines[i].mnemonic == "@") { // ORIGIN

					if (step == 2) {
						if (!obj_list.empty())
							do_create_obj(obj_list, client);
						do_list(list, "\t" + lines[i].mnemonic + "\t" + lines[i].arg + "\t" + lines[i].comment, i);
					}

					stringstream str;
					str << hex << lines[i].arg.substr(1, string::npos);
					str >> initial_addr;
					_ci = initial_addr;
					if (file_number == 0)
						begin_addr = initial_addr;
					continue;
				} else if (lines[i].mnemonic == "$") { // ARRAY
					stringstream str;
					str << hex << lines[i].arg.substr(1, string::npos);
					int s_arr;
					str >> s_arr;
					s_arr &= 0x0fff;
					_ci += s_arr;
					if (step == 2)
						do_list(list, "\t" + lines[i].mnemonic + "\t" + lines[i].arg + "\t" + lines[i].comment, i);
					continue;
				} else if (lines[i].mnemonic == "K") { // CONSTANT
					if (lines[i].label == "") {
						cout << "Assembler Error: Constant without label" << endl;
						exit(1);
					}
					stringstream str;
					str << hex << lines[i].arg.substr(1, string::npos);
					int k;
					str >> k;
					k &= 0x00ff;
					if (step == 2) {
						obj_list.push_back(obj((k << 8) & 0xf000, (k & 0x000f) << 8, true));
						do_list(list, lines[i], i, obj_list.back());
						block_size += 1;
						continue;
					}
					_ci += 1;
				} else if (lines[i].mnemonic == "#") { // END
					if (step == 2) {
						do_create_obj(obj_list, client);
						do_list(list, "\t" + lines[i].mnemonic + "\t" + lines[i].arg + "\t" + lines[i].comment, i);
						continue;

					}
				}
			}

			// Processamento dos operandos (se existirem)
			if (lines[i].arg == "") // Malformed Instruction
				exit(1);

			// Verifica se o operando é um label ou um endereço direto
			if (lines[i].arg[0] == '/') { // Is literal hex
				if (step == 2) {
					stringstream str;
					str << hex << lines[i].arg.substr(1, string::npos);
					int arg;
					str >> arg;
					if (size == 2)
						obj_list.push_back(obj(op << 4*3, arg & 0x0fff));
					else  
						obj_list.push_back(obj(op << 4*3, (arg & 0x000f) << 8));
						
					do_list(list, lines[i], i, obj_list.back());
				}
			} else { // Is label


				// Suporte para notação de array
				bool isArray = lines[i].arg.find("[") != string::npos && lines[i].arg.find("]") != string::npos;

				map<string, string>::iterator it = simbols_list.find(lines[i].arg);

				if ((step == 1) && (it == simbols_list.end())) { // Undefined simbol
					simbols_list[lines[i].arg] = "undefined";
				}
				

				if (step == 2 && it != simbols_list.end()) { //Label already defined, generate obj code
					stringstream str;
					if (!isArray)
						str << hex << simbols_list[lines[i].arg].substr(1, string::npos);
					else {
						string oLabel = lines[i].arg.substr(0, lines[i].arg.length() - (lines[i].arg.find("]") - lines[i].arg.find("[") + 1));
						stringstream sarg;
						uint16_t arg;
						sarg << hex << simbols_list[oLabel].substr(1, string::npos);
						sarg >> arg;

						stringstream sindex;
						sindex << hex << setfill('0') << setw(2) 
							<< lines[i].arg.substr(lines[i].arg.find("[") + 1,
								lines[i].arg.find("]") - lines[i].arg.find("]") + 1);
						sindex >> index;
						arg += index;
						str << hex << setfill('0') << setw(4) << arg;

						simbols_list[lines[i].arg] = "/" + str.str();

					}

					uint16_t arg;
					str >> arg;
					obj_list.push_back(obj(op << 4*3, arg & 0x0fff));
					do_list(list, lines[i], i, obj_list.back());
				}
			}	

			_ci += size;
			if (step == 2)
				block_size += size;
		}
	}

	// Gera código objeto final (se houver)
	do_create_obj(obj_list, client);

	cout << endl << "LABELS" << "\t" << "VALUE" << endl;
	cout << "==========================" << endl;

	for (map<string, string>::iterator it = simbols_list.begin(); it != simbols_list.end(); ++it) {
		cout << it->first << "\t" << hex << it->second << endl;
	}


	cout << endl << "ADDRES\t" << "OBJECT\t" << "LINE\t" << "SOURCE\t" << endl;
	for (int i = 0; i < list.size(); i++) {
		cout << list[i] << endl;
	}
	
	return assembled(file_number, begin_addr, file_name);

}
```
Os métodos auxiliares utilizados pelo assembler não serão aqui expostos, mas seus nomes podem sugerir claramente suas funções. O código fonte do assembler, assim como todos os outros, se encontram no site do projeto.

Nota-se que o tipo de retorno é `assembled`, uma estrutura de dados que contém a seguinte definição:
```cpp
struct assembled {
	int size;
	unsigned int initial_addres;
	string name;
	assembled(int s, unsigned int i, string n) 
		: size(s)
		, initial_addres(i) 
		, name(n.substr(0, n.length() -4 )) {}
};
```

Contendo seu tamanho, endereço inicial e nome, utilizados pelo CLI para o funcionamento da máquina virtual.

### ARQUIVOS GERADOS E FORMATO DO CÓDIGO OBJETO ###

São gerados, dentro da pasta bin, dois tipos de arquivos com a seguinte nomenclatura: nomeX.o e nomeX.bin, sendo X um número inteiro positivo ou zero, representado o número de blocos gerados pela montagem do código. Os arquivos do tipo `.o` apresentam o código objeto em hexadecimal, para leitura humana. Já os arquivos do tipo `.bin` apresentam-no em formato binário, pronto para ser carregado em memória.

Estes arquivos seguem um formato bem definido de código objeto, sendo que apresenta a seguinte estrutura:

* Primeiro e segundo bytes: Endereço inicial de carregamento do programa em memória.
* Terceiro byte: Tamanho do programa (em bytes)
* Último byte: Chechsum
* Demais bytes: Dados e instruções

## INTERFACE DE LINHA DE COMANDOS (CLI) ##

### DESCRIÇÃO ###

Para efetuar o controle dos macro componentes do sistema, isto é: Máquina Virtual e Assembler, foi implementado um sistema de interface por linha de comando com sistema de login e possibilitando a separação de áreas de usuário, com seus respectivos programas.
O cli atua como orquestrador entre o usuário e os sistemas envolvidos, fazendo com que a utilização do sistema se torne mais transparente e fácil para o usuário.

### LOGIN ###

O sistema apresenta um sistema bastante simples de login, o qual se baseia em um arquivo `passwd` na pasta root do projeto, contendo nome de usuário e senha, que dever ser inseridos ao iniciar o sistema de programação.
Este nome de usuário é utilizado para separar os ambientes de usuários, de forma que cada usuário tem um sub-diretório dentro do diretório `usr`. Na pasta do usuário encontram-se os diretórios bin/ e src/, sendo que o primeiro recebe os arquivos de saída do Montador e o segundo os códigos fonte dos programas do usuário.

### COMANDOS DA INTERFACE ###

O CLI apresenta os seguintes comandos:

1. $HELP

Exibe menu de ajuda com os comandos disponíveis.

2. $DIR

Exibe os programas fonte disponíveis no diretório do usuário.

3. $DEL <nome>

Remove da listagem de programas disponíveis e marca os arquivos da pasta bin/ associados a <nome> para serem deletados ao final da execução da CLI

4. $RUN <nome>

Executa um programa que esteja disponível na pasta src/, efetuando sua montagem e executando a máquina virtual no endereço necessário.

5. $DUMP <nome>

Executa o programa Dumper e salva os resultados com o nome especificado.

6. $END <nome>

Finaliza a execução do CLI e efetua a remoção dos arquivos marcados para serem deletados. 

## PROGRAMAS ESCRITOS EM LINGUAGEM SIMBÓLICA ##

Para demonstração das funcionalidades da linguagem simbólica e completude do sistema de programação foram codificados alguns programas de sistema e programas para teste.

### LOADER ###

```
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
	JZ	END	;Se SUM = 0 finaliza
	CN	/00	;Caso contraio HALT MACHINE
END	CN	/02	;Ativa modo indireto
			;JP	START	;Salta para o inicio do programa carregado
	CN	/00	;Halt Machine
IADDR	K	/00
IADDR2	K	/00
SIZE	K	/00
ONE	K	/01
START	K	/00
START2	K	/00
SUM	K	/00
	#	INIT
```

### DUMPER ###
```
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
```

Nota-se que o programa espera um Input de tres bytes, sendo eles: primeiro e segundos bytes do endereço a partir do qual o Dumper deve começar a atuar e terceiro byte sendo a quantidades de dados a serem descarregados.

### VALOR QUADRADO ###

```
INICIO	@	/0300 	;N2 - Calcula o quadrado de um número
	OS	/01	;Habilita trace mode
	IO	/01	;Efetua a leitura de um byte
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

```

## Testes e Resultados ##

### DEMONSTRAÇÃO FUNCIONAMENTO DE FLUXO DE EXECUÇÃO CLI ###

1. Boas Vindas e Login:
```
  ____  _     ____
 / ___|(_)___|  _ \ _ __ ___   __ _
 \___ \| / __| |_) | '__/ _ \ / _` |
  ___) | \__ \  __/| | | (_) | (_| |
 |____/|_|___/_|   |_|  \___/ \__, |
                              |___/

Rodrigo Perrucci Macharelli

User: rpm
Senha: rpm
```

2. Tela Inicial e Menu de Ajuda:
```
Digite $HELP para obter os comandos disponíveis.

rpm@SisProg$ 
```

Ao digitar o comando $HELP, como sugerido, temos:
```
rpm@SisProg$ $HELP

Comandos disponíveis: 
$HELP
	Apresenta este diálogo
$DIR
	Exibe lista de programas do usuário atual
$DEL <nome>
	Remove do sistema de programação o acesso ao programa especificado
$RUN <nome>
	Inicializa a maquina virtual e executa o programa especificado
$DUMP <nome>
	Executa o programa Dumper
$END
	Finaliza a operação do CLI e deleta os arquivos necessários na máquina hospedeira

```

3. Exemplo Funcionamento Dir

```
rpm@SisProg$ $DIR


0. n2-array.asm
1. n2-rpm.asm
2. test-trace.asm
3. test.asm
```

4. Exemplo $RUN 

Para exemplificar o funcionamento do comando $RUN utiliza-se o programa n2-rpm.asm, localizado dentro da pasta de usuário 'rpm'.

```
Digite $HELP para obter os comandos disponíveis.

rpm@SisProg$ $RUN n2-rpm.asm

Machine is halted
Acumulador       : | _acc: 0
Machine is halted
Acumulador       : | _acc: 0
Input: 07
Machine is halted
Acumulador       : | _acc: 49
```
Há algumas observações importantes sobre esse resultado. A primeira é o funcionamento correto da máquina, obtendo o valor 49 no acumulador ao final de sua execução para o input (digitado via terminal) 0x07. Nota-se também que a máquina para três vezes. Isso evidencia o funcionamento da máquina virtual (exposto no primeiro capítulo) executando a montagem e load do 'loader.asm' e 'n2-rpm.asm'.

5. Exemplo $DEL

Os resultados abaixo mostram o funcionamento do comando DEL, que remove um programa da lista de programas executáveis. Neste exemplo deleta-se o programa 'test.asm'.

```
rpm@SisProg$ $DIR


0. n2-array.asm
1. n2-rpm.asm
2. test-trace.asm
3. test.asm


rpm@SisProg$ $DEL test.asm
rpm@SisProg$ $DIR


0. n2-array.asm
1. n2-rpm.asm
2. test-trace.asm


rpm@SisProg$ $RUN test.asm
Programa não encontrado
```

6. Exemplo $DUMP

O programa dumper pode ser acessado via CLI, que passa a execução à máquina virtual. O seu funcionamento espera o endereço inicial a ser lido em dois bytes de input. Em seguida pede a quantidade de dados em byte que dever ser despejados da memória. A máquina virtual então executa o dumper e o arquivo 'nome.dmp.bin' é gerado.
Observando o arquivo '.o' gerado na pasta `./usr/rpm/bin` contém a seguinte informação:

```
rpm@rpm:~/Poli/SisProg/Projetos/usr/rpm/bin$ cat n2-rpm0.o
01 00 26
b0 c1 95 00 11 22 85 01 
95 03 95 04 85 00 55 01 
95 00 11 22 85 03 45 02 
95 03 85 04 45 03 95 04 
01 0a 85 04 c0 30 

f1
```

Note que os três primeiros bytes são justamente os inputs que devemos inserir no programa dumper para obter o dump dessa região da memória na qual o programa n2-rpm.asm está alocado por ter sido rodado nos exemplos anteriores.

```
rpm@SisProg$ $DUMP dump-test

Machine is halted
Acumulador       : | _acc: 0
Input: 01
Input: 00
Input: 26
Machine is halted
Acumulador       : | _acc: -15
```

O arquivo dump-test.dmp.bin é gerado na pasta bin. Espera-se que o arquivo binário gerado seja identico ao arquivo 'n2-rpm0.bin', uma vez que fizemos a cópia da memória que contém este programa e o checksum deve ser gerado pelo dumper. Para verificar se são efetivamente identicos utiliza-se o programa `diff` do linux, para comparar os binários:

```
rpm@rpm:~/Poli/SisProg/Projetos/usr/rpm/bin$ diff n2-rpm0.bin dump-test.dmp.bin
rpm@rpm:~/Poli/SisProg/Projetos/usr/rpm/bin$ 
```
A ausência de respostas indica que os arquivos são iguais.

7. Exemplo $END

O comando END finaliza a execução do CLI e remove os arquivos '.o' e '.bin' associados a todos os programas que tenham sido removidos do sistema pelo comando $DEL.

Veja como se encontra a pasta `./usr/rpm/bin`

```
rpm@rpm:~/Poli/SisProg/Projetos/usr/rpm/bin$ ls | grep test
test0.bin
test0.o
test1.bin
test1.o
```

Os arquivos relativos ao programa `test.asm`, que foi removido, ainda se encontram na pasta. Executa-se o comando $END no CLI:

```
rpm@SisProg$ $END
rpm@rpm:~/Poli/SisProg/Projetos$
```

Novamente a pasta `./usr/rpm/bin':

```
rpm@rpm:~/Poli/SisProg/Projetos/usr/rpm/bin$ ls | grep test
rpm@rpm:~/Poli/SisProg/Projetos/usr/rpm/bin$ 
```

Note que os arquivos referentes ao program test.asm foram deletados da máquina hospedeira ao fim da execução do CLI.

## PROGRAMAS ESCRITOS EM LINGUAGEM SIMBÓLICA ##

### LOADER ###

1. Código Fonte

```
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
	JZ	END	;Se SUM = 0 finaliza
	CN	/00	;Caso contraio HALT MACHINE
END	CN	/02	;Ativa modo indireto
			;JP	START	;Salta para o inicio do programa carregado
	CN	/00	;Halt Machine
IADDR	K	/00
IADDR2	K	/00
SIZE	K	/00
ONE	K	/01
START	K	/00
START2	K	/00
SUM	K	/00
	#	INIT

```

2. Listagem e Labels gerado pelo Assembler

```
LABELS	VALUE
==========================
CHECK	/0032
END	/003a
IADDR	/003c
IADDR2	/003d
INIT	/0001
LOOP	/001a
ONE	/003f
SIZE	/003e
START	/0040
START2	/0041
SUM	/0042

ADDRES	OBJECT	LINE	SOURCE	
		1		@	/0001	;Endereço inicial do loader

0001	c2	2	INIT	IO	/02	;GET DATA - Device 2 (file)

0002	903c	3		MM	IADDR	;Salva primeiro byte do endereço inicial do programa

0004	9040	4		MM	START	;Salva novamente para termos o endereço ao final

0006	4042	5		+	SUM	;Soma o byte para checksum

0008	9042	6		MM	SUM	

000a	c2	7		IO	/02	;GET DATA

000b	903d	8		MM	IADDR2	;Salva o segundo byte do endereço inicial do programa

000d	9041	9		MM	START2	

000f	4042	10		+	SUM	;Soma o byte para checksum

0011	9042	11		MM	SUM	

0013	c2	12		IO	/02	;GET DATA

0014	903e	13		MM	SIZE	;Salva o tamanho do programa (em bytes)

0016	4042	14		+	SUM	;Soma o byte para checksum

0018	9042	15		MM	SUM	

001a	c2	16	LOOP	IO	/02	;GET DATA

001b	32	17		CN	/02	;Ativa modo indireto

001c	903c	18		MM	IADDR	;Guarda (indiretamente) o byte lido no endereço atual 

001e	4042	19		+	SUM	;Soma o byte para checksum

0020	9042	20		MM	SUM	

0022	803d	21		LD	IADDR2	;Carrega o endereço atual

0024	403f	22		+	ONE	;Atualiza (soma 1)

0026	903d	23		MM	IADDR2	;Salva

0028	803e	24		LD	SIZE	;Carrega o tamanho atual

002a	503f	25		-	ONE	;Atualiza (subtrai 1)

002c	903e	26		MM	SIZE	;Salva

002e	1032	27		JZ	CHECK	;Se SIZE = 0 pula para o checksum

0030	1a	28		JP	LOOP	;Caso contrario, continua lendo

0032	c2	29	CHECK	IO	/02	;GET DATA

0033	4042	30		+	SUM	;Soma o byte de checksum ao valor da soma atual

0035	9042	31		MM	SUM	

0037	103a	32		JZ	END	;Se SUM = 0 finaliza

0039	30	33		CN	/00	;Caso contraio HALT MACHINE

003a	32	34	END	CN	/02	;Ativa modo indireto

		35	;JP START ;Salta para o inicio do programa carregado

003b	30	36		CN	/00	;Halt Machine

003c	0	37	IADDR	K	/00	

003c	0	38	IADDR2	K	/00	

003c	0	39	SIZE	K	/00	

003c	1	40	ONE	K	/01	

003c	0	41	START	K	/00	

003c	0	42	START2	K	/00	

003c	0	43	SUM	K	/00	

		44		#	INIT
```

### DUMPER ###

1. Código Fonte

```
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
```

2. Listagem e Labels gerado pelo Assembler

```
LABELS	VALUE
==========================
CHECK	/0080
COMP	/008c
IADDR	/0088
IADDR2	/0089
INIT	/0050
LOOP	/0068
ONE	/008b
SIZE	/008a
START	/008d
START2	/008e
SUM	/008f

ADDRES	OBJECT	LINE	SOURCE	
		1		@	/0050	;Endereço inicial do dumper 

0050	c1	2	INIT	IO	/01	;GET DATA - Device 1 (stdin) - Primeiro Byte

0051	9088	3		MM	IADDR	;Salva primeiro byte do endereço inicial do programa

0053	c6	4		IO	/06	;PUT DATA - Device 2 (file)

0054	408f	5		+	SUM	;Soma o byte para checksum

0056	908f	6		MM	SUM	

0058	c1	7		IO	/01	;GET DATA - Device 1 (stdin) - Segundo Byte

0059	9089	8		MM	IADDR2	;Salva o segundo byte do endereço inicial do programa

005b	c6	9		IO	/06	;PUT DATA - Device 2 (file)

005c	408f	10		+	SUM	;Soma o byte para checksum

005e	908f	11		MM	SUM	

0060	c1	12		IO	/01	;GET DATA - Device 1 (stdin) - Tamanho do programa

0061	908a	13		MM	SIZE	;Salva o tamanho do programa (em bytes)

0063	c6	14		IO	/06	;PUT DATA - Device 2 (file)

0064	408f	15		+	SUM	;Soma o byte para checksum

0066	908f	16		MM	SUM	

0068	32	17	LOOP	CN	/02	;Ativa modo indireto

0069	8088	18		LD	IADDR	;Carrega (indiretamente) o byte lido do endereço atual 

006b	c6	19		IO	/06	;PUT DATA - Device 2 (file)

006c	408f	20		+	SUM	;Soma o byte para checksum

006e	908f	21		MM	SUM	

0070	8089	22		LD	IADDR2	;Carrega o endereço atual

0072	408b	23		+	ONE	;Atualiza (soma 1)

0074	9089	24		MM	IADDR2	;Salva

0076	808a	25		LD	SIZE	;Carrega o tamanho atual

0078	508b	26		-	ONE	;Atualiza (subtrai 1)

007a	908a	27		MM	SIZE	;Salva

007c	1080	28		JZ	CHECK	;Se SIZE = 0 pula para o checksum

007e	68	29		JP	LOOP	;Caso contrario, continua lendo

0080	808c	30	CHECK	LD	COMP	;Carrega FF no acumulador

0082	508f	31		-	SUM	;Efetua FF - SUM

0084	408b	32		+	ONE	;Soma um

0086	c6	33		IO	/06	;PUT DATA - Device 2 (file)

0087	30	34		CN	/00	;Halt Machine

0088	0	35	IADDR	K	/00	

0088	0	36	IADDR2	K	/00	

0088	0	37	SIZE	K	/00	

0088	1	38	ONE	K	/01	

0088	ff	39	COMP	K	/FF	

0088	0	40	START	K	/00	

0088	0	41	START2	K	/00	

0088	0	42	SUM	K	/00	

		43		#	INIT	
```

### FUNCIONAMENTO TRACE ###

Para exemplificar o funcionamento do modo trace de execução da máquina virtual foi feito o seguinte programa simples em linguagem simbólica

```
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
```

Executando o programa temos o seguinte output:

```
Machine is halted
Acumulador       : | _acc: 0
Machine is halted
Acumulador       : | _acc: 0
[194:
Registradores    : | _ci: 100 | _cb: 2 | _ri: b100
Estado da Maquina: | IN : 0 | HA: 0
Acumulador       : | _acc: 0
]

##########MEM MAP##########
    00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 

00  b1 41 aa 41 ab 41 ac b0 30 00 00 00 00 00 00 00 
01  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
02  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
03  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
04  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
05  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
06  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
07  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
08  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
09  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0a  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0b  00 00 00 00 00 00 01 02 00 00 00 00 00 00 00 00 
0c  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0d  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0e  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0f  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
[c3:
Registradores    : | _ci: 101 | _cb: 2 | _ri: 41aa
Estado da Maquina: | IN : 0 | HA: 0
Acumulador       : | _acc: 0
]

##########MEM MAP##########
    00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 

00  b1 41 aa 41 ab 41 ac b0 30 00 00 00 00 00 00 00 
01  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
02  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
03  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
04  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
05  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
06  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
07  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
08  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
09  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0a  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0b  00 00 00 00 00 00 01 02 00 00 00 00 00 00 00 00 
0c  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0d  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0e  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0f  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
[c4:
Registradores    : | _ci: 103 | _cb: 2 | _ri: 41ab
Estado da Maquina: | IN : 0 | HA: 0
Acumulador       : | _acc: 1
]

##########MEM MAP##########
    00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 

00  b1 41 aa 41 ab 41 ac b0 30 00 00 00 00 00 00 00 
01  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
02  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
03  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
04  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
05  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
06  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
07  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
08  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
09  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0a  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0b  00 00 00 00 00 00 01 02 00 00 00 00 00 00 00 00 
0c  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0d  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0e  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0f  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
[c5:
Registradores    : | _ci: 105 | _cb: 2 | _ri: 41ac
Estado da Maquina: | IN : 0 | HA: 0
Acumulador       : | _acc: 3
]

##########MEM MAP##########
    00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 

00  b1 41 aa 41 ab 41 ac b0 30 00 00 00 00 00 00 00 
01  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
02  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
03  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
04  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
05  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
06  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
07  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
08  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
09  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0a  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0b  00 00 00 00 00 00 01 02 00 00 00 00 00 00 00 00 
0c  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0d  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0e  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0f  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
Machine is halted
Acumulador       : | _acc: 3
```

Repare que as informações da execução da máquina passa a ser impressa no terminal, mostrando os valores de todos os registradores, estado da máquina (halted ou modo indireto) e o valor do acumulador.
No mapa de memória temos o programa carregado a partir de 0000 (endereço relativo ao valor inicial do programa), contendo os dados da memória em formato hexadecimal.
A partir de 0x0b05 temos os três dados carregados com o programa: 0x00, 0x01, 0x02.

## CONSIDERAÇÕES FINAIS ##

### CÓDIGO FONTE E COMPILAÇÃO ###

Todo o código fonte do projeto pode ser encontrado em um repositório do github, cujo link está presente no site do projeto.
O código pode ser compilado e executado tanto em sistemas Windows quanto em sistemas GNU Linux.
O arquivo 'compile.sh' pode ser utilizado para compilar o arquivo fonte em sistemas Linux.
Todo o projeto pode ser compilado utilizando o compilador 'g++', sem necessidade de flags adicionais.

### POSSÍVEIS MELHORIAS ###

A implementação de um conjunto de instruções mais flexível e completo poderia facilitar a programação em baixo nível.

O Assembler pode ser adaptado para lidar com códigos relocáveis, não contemplados neste projeto.

As impementações do sistema de interrupção podem ser efetuadas para a completude do sistema de programação.

