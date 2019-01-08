#include <iostream>
#include "VM.cpp"
#include "Assembler.cpp"

using namespace std;

int main(int argc, char** argv)
{
	Assembler *as = new Assembler();
	as->Assemble("loader.asm");
	assembled obj = as->Assemble("n2.asm");
	VM *vm = new VM();
	vm->start();
	vm->load("n2", obj.size);
	vm->run(obj.initial_addres & 0xffff, true);
	
	delete vm;
	delete as;

	return 0;
}
