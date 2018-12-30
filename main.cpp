#include <iostream>
#include "VM.cpp"
#include "Assembler.cpp"

using namespace std;

int main(int argc, char** argv)
{
	Assembler *as = new Assembler();
	as->Assemble("loader.asm");
	as->Assemble("test.asm");

	VM *vm = new VM();
	vm->start();
	vm->run();
	
	delete vm;
	delete as;

	return 0;
}
