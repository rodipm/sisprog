#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#define banks 16
#define bank_size 4096
#define steps 100
#define DEBUG true

using namespace std;

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

class VM 
{
	// Banco de memórias
	uint8_t mem[banks][bank_size];

	// Registradores
	int16_t _acc;
	uint8_t _cb;
	uint16_t _ri, _ci;

	// Tabela de tamanhos das instruções
	const int instruction_sizes[0xd] = {2,2,2,1,2,2,2,2,2,2,2,1,1};
	
	// Variáveis de estado da maquina virtual e controle	
	bool running;
	bool indirect;
	int step;
	update_ci_info *ci_info;
	
	// Io devices array
	// Podem ser referenciados 3 devices pela instrucao IO, ja que o primeiro referencia todos
	// o primeiro par é reservado para stdin e stdout.
	// os dois ultimos ficam lires para serem utilizados.
	// [0] = [cin, cout] \x1
	// [1] = [nullptr, nullptr] \x2
	// [2] = [nullptr, nullptr] \x3
	streambuf *io_devices[3][2];

	// Stream para read e write (inicializada com nullptr)
	iostream *stream;	
	
	// Member Instruction Pointer para guardar a instrução a ser executada (desacoplando fetch e decode)
	void (VM::*execute)(void);
public:
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

		// Demais inicializações
		running = false;
		indirect = false;
		step = 0;
		ci_info = new update_ci_info();

		// Inicialização do device padrao (stdin e stdout)
		stream = new iostream(nullptr);
		io_devices[0][0] = cin.rdbuf();
		io_devices[0][1] = cout.rdbuf();
		for (int i = 1; i < 3; i++)
			for (int j = 0; j < 2; j++)
				io_devices[i][j] = nullptr;

		/*****DEBUG*****/
		/*****DEBUG*****/
		/*****DEBUG*****/
		/*****DEBUG*****/

		mem[0][0x00] 	= 0x30; //<- Halt Machine
		mem[0][0x01] 	= 0x32; //TODO <- Loader
		mem[0][0x02] 	= 0x80;
		mem[0][0x03] 	= 0xfe;
		mem[0][0x04] 	= 0x32;
		mem[0][0x05] 	= 0x40;
		mem[0][0x06] 	= 0xfe;
		mem[0][0x07] 	= 0x00;
		mem[0][0x08] 	= 0x01;
		mem[0][0xfe] 	= 0xff;
		mem[0][0xff] 	= 0xff;
		mem[0xf][0xfff]	= 0x5;
	}
	
	~VM() {
		delete ci_info;
		delete stream;
	}

/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

	void update_ci(uint8_t size) {
		if (!ci_info->redirected)
			_ci += size;
		else {
			_ci = ci_info->addr;
			ci_info->redirected = false;
		}
	}

	// retorna o endereço contido em uma celula de memória referenciada por acesso indireto à mem.
	uint16_t getIndirectPtr() {
		uint16_t OP = _ri & 0x0fff;
		uint16_t ptr  = ((uint16_t)mem[_cb][OP] << 8) & 0xff00 | ((uint16_t)mem[_cb][OP + 1] & 0x00ff);
		return ptr;
	}	
	
	void debug(uint8_t instructionSize) {
		if (DEBUG) {
			cout << "[" << step << ":" << endl;
			cout << "Registradores    :" << hex <<" | _ci: " << _ci << " | _cb: " << (uint16_t)_cb << " | _ri: " << _ri << endl;
			cout << "Estado da Maquina:" << " | IN : " << indirect << " | HA: " << !running << endl;
			cout << "Acumulador       :" << " | _acc: " << dec << _acc << endl;
			cout << "]" <<  endl;
		}
	}
		
	
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

	// Instrução JUMP UNCONDITIONAL 0x0XXX
	void JP() {
		if (!indirect) {
			ci_info->addr	= _cb << 4;
			ci_info->addr	= ci_info->addr << 4*3;
			ci_info->addr	|= _ri & 0x0fff;

		}
		else {
			uint16_t OP = _ri & 0x0fff;
			uint16_t ptr  = getIndirectPtr();
			_cb = (uint8_t)(ptr >> 4*3);
			indirect = false;
			ci_info->addr = ((_cb << 4) << 4*3 ) | (ptr & 0x0fff);
		}
		ci_info->redirected = true;
	}
	
	// Instrução JUMP IF ZERO 0x1XXX
	void JZ() {
		if (_acc == 0)
			this->JP();
	}

	// Instrução JUMP IF NEGATIVE 0x2XXX
	void JN() {
		if (_acc < 0)
			this->JP();
	}

	// Instrução CONTROLE 0x3X
	void CN() {
		switch((_ri & 0x0f00) >> 8) {
			case 0x0: // HLT
				cout << "Machine is halted" << endl;
				running = false;
				break;
			case 0x1: // RI
				cout << "Return from interrupt" << endl;
				break;
			case 0x2: // IN
				cout << "Indirect mode ON" << endl;
				indirect = true;
				break;
			case 0x3: // NOP;
				cout << "NOP" << endl;
				break;
			default:
				cout << "Fail!" << endl;
				break;
		}	
	}

	// Instrução PLUS 0x4XXX
	void plus() {
		if (!indirect) {
			_acc += (int8_t)mem[_cb][_ri & 0x0fff];
		}
		else {
			uint16_t ptr = getIndirectPtr();
			_acc += (int8_t)mem[ptr >> 4*3][ptr & 0x0fff];
			indirect = false;
		}	
	}
	
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

	// Instrução LOAD FROM MEMORY 0x8XXX
	void LD() {
		if (!indirect) {
			_acc = (int8_t)mem[_cb][_ri & 0x0fff];
		}	
		else {
			uint16_t ptr = getIndirectPtr();
			_acc = (int8_t)mem[ptr >> 4*3][ptr & 0x0fff];
			indirect = false;
		}	
	}

	// Instrução MOVE TO MEMORY 0x9XXX
	void MM() {

		if (!indirect) {
			mem[_cb][_ri & 0x0fff] = (uint16_t)_acc;
		}	
		else {
			uint16_t ptr = getIndirectPtr();
			mem[ptr >> 4*3][ptr & 0x0fff] = (uint16_t)_acc;
			indirect = false;
		}	
	}
	// Instrução SUBROTINE CALL 0xAXXX
	void SC() {
		// A instrução deve guardar o endereço de retorno em \yxxx (com base em _cb) e executar a próxima instrução (\yxxx + 0x2)
		uint16_t ret_addr = _ci;
		mem[_cb][_ri & 0x0fff] = (uint8_t)ret_addr;
		mem[_cb][(_ri & 0x0fff) + 1] = (uint8_t)(ret_addr);
		ci_info->addr = (_cb << 4) | (_ri & 0x0fff);  
		ci_info->redirected = true;
		
	}

	// Instrução de chamada do sistema operacional 0xBX
	void OS() {
		// Empty
	}

	// Instrução INPUT OUTPUT 0xCX
	void IO() {
		// Operando (4 ultimos bits)
		uint16_t OP = ((_ri & 0x0f00) >> 8);
		// Device a ser operado
		uint16_t DEV = OP & 0x3; // 0x3 = 0b0011 -> mascara para os dois últimos bits apenas
		
		// Para transformar int em char *, primeiramente transformamos em string e depois em c_str (char *).
		stringstream str;
		str << _acc;

		// Processamento do OP para verificar que tipo de instrução de IO deve ser eecutada
		// verifica os dois bits mais significativos dos ultimos 4 bits da instruçao
		// PUT DATA
		if (OP & 0x4) { // 0b0100
			if (DEV == 0x0) {
				for (int i = 0; i < 3; i++) {
					if (io_devices[i][1] != nullptr) {
						stream->rdbuf(io_devices[i][1]);
						stream->write(str.str().c_str(), sizeof(int16_t));
						stream->flush();	
					}
				}
					
			}
			else{
				if (io_devices[DEV-1][1] != nullptr) {
					stream->rdbuf(io_devices[DEV-1][1]);
					stream->write(str.str().c_str(), sizeof(int16_t));
					stream->flush();
				}
			}
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
					char buff[1];
					stream->rdbuf(io_devices[DEV-1][0]);
					stream->read(buff, 1);
					// Apenas fará sentido se o input for o stdin.
					cin.ignore(120, '\n');
					_acc = (int16_t)buff[0] >= 97 ? (int16_t)buff[0] - 'a' + 0xa : (int16_t)buff[0] - '0';
				}
			}
		}
	}

/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

	// Busca a instrução a ser executada baseada em _ci e a armazena em _ri
	int fetch() {
		// Verifica o CO da instrução para saber qual o seu tamanho
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

	// Interpreta a instrução e encaminha para a execução correta
	uint16_t decode() {
		// Obtem o CO da instrução
		uint16_t CO = _ri >> 4*3;

		// garante que o modo indireto só se mantem em caso de instrução de acesso à memóra
		if ((CO == 0x3 || CO > 0xa) && indirect)
			indirect = false;

		switch(CO) {
			case 0x0: //JP
				//this->JP();
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
				break;
		}
		return -1;
	}

	// Inicializações do sistema
	void start() {
		step = 0;
		running = true;
	}
	
	// TODO Step mode
	// Inicia a maquina
	void run() {
		while (step < steps && running) {
			int instructionSize = this->fetch();
			this->decode();
			(this->*execute)();
			debug(instructionSize);

			//atualiza o _ci
			update_ci(instructionSize);

			step++;
		}
	}

};

