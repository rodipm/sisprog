#include <iostream>
#include "VM.cpp"
#include "Assembler.cpp"

int main(int arcg, char** argv)
{

	VM *vm = new VM();
	//vm->start();
	//vm->run();
	
	Assembler *as = new Assembler();

	delete vm;
	delete as;

	return 0;
}
