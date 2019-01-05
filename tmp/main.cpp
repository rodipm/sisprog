#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

int main() {
	char check = 0x00; 
	stringstream ss;
	ss << (char)0xff;
	ss >> check;
	cout << hex << ss.str() << endl;	
	cout << hex << ((unsigned short int)check & 0x00ff + 1) << endl;
	return 0;
}
