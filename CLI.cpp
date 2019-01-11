#include <iostream>
#include <cstdio>
#include <dirent.h>
#include <map>
#include "VM.cpp"
#include "Assembler.cpp"

#ifdef _WINDOWS
#include <windows.h>
#define CLEAR "cls"
#else
#include <unistd.h>
#define CLEAR "clear"
#define Sleep(x) usleep((x)*1000)
#endif

string client;
VM *vm;
Assembler *as;
map<string, bool> programs;

void welcome() {

	cout << R"(
  ____  _     ____
 / ___|(_)___|  _ \ _ __ ___   __ _
 \___ \| / __| |_) | '__/ _ \ / _` |
  ___) | \__ \  __/| | | (_) | (_| |
 |____/|_|___/_|   |_|  \___/ \__, |
                              |___/)" << endl;

	cout << endl << "Rodrigo Perrucci Macharelli" << endl;
	Sleep(2000);
	system(CLEAR);

}

bool login () {
	cout << "User: ";
	string user;
	cin >> user;
	cout << "Senha: ";
	string senha;
	cin >> senha;
	cin.ignore(256, '\n');
	fstream file("passwd", ios_base::in);
	
	while (file) {
		string u, p;
		file >> u >> p;
		if (user.compare(u) == 0 && senha.compare(p) == 0) {
			client = user;
			return true;
		}
	}
	return false;
}

void help () {
	cout << endl << "Comandos disponíveis: " << endl;
	cout << "$HELP" << endl << "\t" << "Apresenta este diálogo" << endl;
	cout << "$DIR" << endl << "\t" << "Exibe lista de programas do usuário atual" << endl;
	cout << "$DEL <nome>" << endl << "\t" << "Remove do sistema de programação o acesso ao programa especificado" << endl;
	cout << "$RUN <nome>" << endl << "\t" << "Inicializa a maquina virtual e executa o programa especificado" << endl;
	cout << "$END" << endl << "\t" << "Finaliza a operação do CLI e deleta os arquivos necessários na máquina hospedeira" << endl;
	cout << endl << endl;
}

void run (string program) {
	if (programs.find(program) == programs.end() || !programs[program]) {
		cout << "Programa não encontrado" << endl;;
		return;
	}
	as->Assemble("loader.asm");
	assembled prog = as->Assemble(program);
	cout << "ok" << endl;
	vm->start();
	vm->load(prog.name, prog.size);
	vm->run(prog.initial_addres & 0xffff, true);
}

void dir (bool show = false) {
	DIR *pdir = nullptr;
	struct dirent *pent = nullptr;

	pdir = opendir(".");
	if (pdir == nullptr) exit(1);

	while (pent = readdir(pdir)) {
		if (pent == NULL) exit(1);

		string name(pent->d_name);

		if (name.find(".asm") != string::npos && programs.find(name) == programs.end() && name.find(".asm") == name.length() - 4)	
			programs[name] = true;
	}
	
	if (show) {
	unsigned short int index = 0;
		cout << endl << endl;
		for (map<string, bool>::iterator it = programs.begin(); it != programs.end(); ++it)
			if (it->second)
				cout << index++ << ". " << it->first << endl;
		
		cout << endl << endl;
	}
	closedir(pdir);
}

void del (string program) {
	if (programs.find(program) != programs.end())
		programs[program] = false;
}

void ASM (string program) {
	as->Assemble(program);

	if (programs.find(program) == programs.end())
		programs[program] = true;
}

bool interpret (string command) {
	if (command.compare("$HELP") == 0)
		help();
	else if (command.find("$RUN") != string::npos)
		run(command.substr(command.find(' ') + 1, string::npos));
	else if (command.compare("$DIR") == 0)
		dir(true);
	else if (command.find("$DEL") != string::npos)
		del(command.substr(command.find(' ') + 1, string::npos));
	else if (command.compare("$END") == 0)
		return false;
	else if (command.find("$ASM") != string::npos)
		ASM(command.substr(command.find(' ') + 1, string::npos));
	else
		cout << "Comando inválido!" << endl;
	return true;
}

void deleteFiles () {
	DIR *pdir = nullptr;
	struct dirent *pent = nullptr;

	pdir = opendir(".");
	if (pdir == nullptr) exit(1);

	for (map<string, bool>::iterator it = programs.begin(); it != programs.end(); ++it)
		if (!it->second) {
			while (pent = readdir(pdir)) {
				if (pent == NULL) exit(1);

				string name(pent->d_name);

				if (name.find(it->first.substr(0, it->first.length() - 4)) != string::npos)	
					if(remove(name.c_str()))
						cout << "Erro ao deletar o programa " << name << "." << endl;
			}
		}
}

int main (int argc, char **argv) {
	client = "rpm";	
	dir();
	//welcome();
	//while (!login()) {
	//	cout << endl << "Usuário ou senha incorretos!" << endl << endl;
	//}	
	
	vm = new VM();
	as = new Assembler();

	system(CLEAR);

	cout << endl << "Digite $HELP para obter os comandos disponíveis." << endl << endl;
	string command;
	do {
		cout << client << "@" << "SisProg$ ";
		getline(cin, command);	
	}
	while (interpret(command));

	deleteFiles();

	delete vm;
	delete as;

	return 0;
} 
