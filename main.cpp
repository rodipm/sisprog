#include <iostream>
#include "VM.cpp"
int main(int arcg, char** argv)
{

	VM *vm = new VM();
	vm->start();
	vm->run();


	return 0;
}
