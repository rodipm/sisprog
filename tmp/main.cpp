#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

int main() {
	iostream *stream = new iostream(nullptr);
	stream->rdbuf(nullptr);
	streambuf *io_device;
	io_device = cin.rdbuf();
	stream->rdbuf(io_device);
	char buffer[1];
	stream->read(buffer, 1);
	cout << buffer[0] << endl;
	cin.ignore(256, '\n');
	stream->read(buffer, 1);
	cout << buffer[0] << endl;
	return 0;
}
