#include <cstdint>
#include <iostream>

#define banks 16
#define bank_size 4096
#define steps 10
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
public:
	VM() {
		// Inicialização do banco de memórias
		for (int i = 0; i < banks; i++)
			for (int j = 0; j < bank_size; j++)
				mem[i][j] = 0;	
	
		// Inicialização dos registradores
		_acc = 0;
		_cb  = 0;
		_ri = _ci = 0;

		// Demais inicializações
		running = false;
		indirect = false;
		step = 0;

		ci_info = new update_ci_info();

		/*****DEBUG*****/
		/*****DEBUG*****/
		/*****DEBUG*****/
		/*****DEBUG*****/

		mem[0][0] = 0xa0;
		mem[0][1] = 0x04;
		mem[0][2] = 0x33;
		mem[0][3] = 0x30;
		mem[0][4] = 0xff;
		mem[0][5] = 0xff;
		mem[0][6] = 0x40;
		mem[0][7] = 0xff;
		mem[0][8] = 0x32;
		mem[0][9] = 0x00;
		mem[0][0xa] = 0x04;
		mem[0][0xb] = 0x30;
		mem[0][0xff] = 0x01;
		mem[0][0x101] = 0x30;
	}
	
	~VM() {
		if (ci_info != NULL)
			delete ci_info;
	}

	void update_ci(uint8_t size) {
		if (!ci_info->redirected)
			_ci += size;
		else {
			_ci = ci_info->addr + size;
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
		if (DEBUG)
			cout << "[" << step << ":" << endl;
			cout << "Registradores    :" << hex <<" | _ci: " << _ci << " | _cb: " << (uint16_t)_cb << " | _ri: " << _ri << endl;
			cout << "Estado da Maquina:" << " | IN : " << indirect << " | HA: " << !running << endl;
			cout << "Acumulador       :" << " | _acc: " << dec << (uint16_t)_acc << endl;
			cout << "]" <<  endl;

	}
		
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
	
	// Instrução JUMP UNCONDITIONAL
	void JP() {
		if (!indirect) {
			ci_info->addr	= _cb << 4;
			ci_info->addr	= _ci << 4*3;
			ci_info->addr	|= _ri & 0x0fff;
		}
		else {
			uint16_t OP = _ri & 0x0fff;
			uint16_t ptr  = getIndirectPtr();
			_cb = (uint8_t)(ptr >> 4*3);
			indirect = false;
			ci_info->addr = ((_cb << 4) << 4*3 ) | (ptr & 0x0fff);
			ci_info->redirected = true;
		}
	}
	
	void JZ() {
		if (_acc == 0)
			this->JP();
	}

	void JN() {
		if (_acc < 0)
			this->JP();
	}

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
	// Instrução SUBROTINE CALL
	void SC() {
		// A instrução deve guardar o endereço de retorno em \yxxx (com base em _cb) e executar a próxima instrução (\yxxx + 0x2)
		uint16_t ret_addr = _ci;
		mem[_cb][_ri & 0x0fff] = (uint8_t)ret_addr;
		mem[_cb][(_ri & 0x0fff) + 1] = (uint8_t)(ret_addr);
		ci_info->addr = (_cb << 4) | (_ri & 0x0fff);  
		ci_info->redirected = true;
		
	}

	void OS() {
		// Empty
	}

	void IO() {

	}

	uint16_t decode() {
		// Obtem o CO da instrução
		uint16_t CO = _ri >> 4*3;

		// garante que o modo indireto só se mantem em caso de instrução de acesso à memóra
		
		if ((CO == 0x3 || CO > 0xa) && indirect)
			indirect = false;

		switch(CO) {
			case 0x0: //JP
				this->JP();
				break;
			case 0x1: //JZ
				this->JZ();
				break;
			case 0x2: //JN
				this->JN();
				break;
			case 0x3: //CN
				this->CN();
				break;
			case 0x4: // +
				this->plus();
				break;
			case 0x5: // -
				this->minus();
				break;
			case 0x6: // *
				this->multiply();
				break;
			case 0x7: // /
				this->divide();
				break;
			case 0x8: // LD
				this->LD();
				break;
			case 0x9: // MM
				this->MM();
				break;
			case 0xa: //SC
				this->SC();
				break;
			case 0xb: // OS
				this->OS();
				break;
			case 0xc: // IO
				this->IO();
				break;
			default:
				cout << "Instrução inválida" << endl;
				break;
		}
		return -1;
	}

	void start() {
		step = 0;
		running = true;
	}

	void run() {
		// Controle para manter a máquina em modo indireto apenas se próxima instrução for de acesso a memória
		bool reset_indirect = false;
		while (step < steps && running) {
			int instructionSize = this->fetch();
			this->decode();

			debug(instructionSize);

			//atualiza o _ci
			update_ci(instructionSize);

			step++;
		}
	}

};

