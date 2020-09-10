#include <iostream>
#include <fstream> // Files
#include <sstream>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <algorithm> // std::replace
#include <iomanip>

#define WORD uint16_t
#define BYTE uint8_t

using namespace std;

struct mnemonic
{
	string name;
	BYTE size;
	BYTE op;
};

struct file_line
{
	string label;
	string mnemonic;
	string arg;
	string comment;

	file_line(string l, string m, string a, string c) : label(l), mnemonic(m), arg(a), comment(c) {}
};

struct simbol
{
	string name;
	string addr;

	simbol(string n, string a) : name(n), addr(a) {}
};

struct obj
{
	WORD op;
	WORD arg;
	bool pseudo;
	obj(WORD o, WORD a, bool p = false) : op(o), arg(a), pseudo(p) {}
};

struct assembled
{
	int size;
	unsigned int initial_addres;
	string name;
	assembled(int s, unsigned int i, string n)
		: size(s), initial_addres(i), name(n.substr(0, n.length() - 4)) {}
};

class Assembler
{
	// Tabela de mnemonicos
	mnemonic mnemonic_table[0xd];

	// Contador de instruções
	int _ci;
	int initial_addr;
	int block_size;
	int file_number;
	int begin_addr;
	string file_name;
	uint16_t index;

public:
	Assembler()
	{
		block_size = initial_addr = file_number = 0;
		index = 0;

		// Inicialização da tabela de mnemonicos
		mnemonic_table[0x00].name = "JP";
		mnemonic_table[0x00].size = 2;
		mnemonic_table[0x00].op = 0x0;
		mnemonic_table[0x01].name = "JZ";
		mnemonic_table[0x01].size = 2;
		mnemonic_table[0x01].op = 0x1;
		mnemonic_table[0x02].name = "JN";
		mnemonic_table[0x02].size = 2;
		mnemonic_table[0x02].op = 0x2;
		mnemonic_table[0x03].name = "CN";
		mnemonic_table[0x03].size = 1;
		mnemonic_table[0x03].op = 0x3;
		mnemonic_table[0x04].name = "+";
		mnemonic_table[0x04].size = 2;
		mnemonic_table[0x04].op = 0x4;
		mnemonic_table[0x05].name = "-";
		mnemonic_table[0x05].size = 2;
		mnemonic_table[0x05].op = 0x5;
		mnemonic_table[0x06].name = "*";
		mnemonic_table[0x06].size = 2;
		mnemonic_table[0x06].op = 0x6;
		mnemonic_table[0x07].name = "/";
		mnemonic_table[0x07].size = 2;
		mnemonic_table[0x07].op = 0x7;
		mnemonic_table[0x08].name = "LD";
		mnemonic_table[0x08].size = 2;
		mnemonic_table[0x08].op = 0x8;
		mnemonic_table[0x09].name = "MM";
		mnemonic_table[0x09].size = 2;
		mnemonic_table[0x09].op = 0x9;
		mnemonic_table[0x0a].name = "SC";
		mnemonic_table[0x0a].size = 2;
		mnemonic_table[0x0a].op = 0xa;
		mnemonic_table[0x0b].name = "OS";
		mnemonic_table[0x0b].size = 1;
		mnemonic_table[0x0b].op = 0xb;
		mnemonic_table[0x0c].name = "IO";
		mnemonic_table[0x0c].size = 1;
		mnemonic_table[0x0c].op = 0xc;
	}

	~Assembler()
	{
	}

	vector<file_line> processFile(string file, string client)
	{
		// Abre o arquivo .asm para ser lido
		stringstream dir;
		if (client != "")
			dir << "./usr/" << client << "/src/";
		dir << file;
		fstream input;
		input.open(dir.str(), ios_base::in);

		if (!input)
			exit(1);

		vector<file_line> lines;
		string command, comment;
		string label, mnemonic, arg;

		for (string line; getline(input, line);)
		{
			// Reset strings every iteration
			label = mnemonic = arg = "";

			// Replace tabs with spaces
			replace(line.begin(), line.end(), '\t', ' ');

			if (line.empty())
				break;

			if (line.find(';') != string::npos)
			{ // contém comentário
				comment = line.substr(line.find(';'), string::npos);
				command = line.substr(0, line.find(';') - 1);
			}
			else
			{
				command = line;
				comment = "";
			}
			size_t pos = 0;

			if (line[0] != ' ')
			{ // Has label
				pos = command.find(' ');
				label = command.substr(0, pos);
			}

			if (label == " ")
				label = "";

			// Get MNEMONIC (or pseudo)
			size_t newPos = command.find(' ', pos + 1);

			mnemonic = command.substr(pos + 1, newPos - pos - 1);
			if (mnemonic == " " || mnemonic == "\t")
				mnemonic = "";

			if (newPos + 1 < command.length()) // Has operand
				arg = command.substr(newPos + 1, command.length());
			if (arg == " " || arg == "\t")
				arg = "";

			file_line current_line(label, mnemonic, arg, comment);
			lines.push_back(current_line);
		}

		return lines;
	}

	void do_list(vector<string> &list, string line, int lineno)
	{
		stringstream str;
		str << "\t"
			<< "\t" << lineno + 1 << "\t" << line << endl;
		list.push_back(str.str());
	}

	void do_list(vector<string> &list, file_line &line, int lineno, obj &o)
	{
		stringstream str;
		str << hex << setfill('0') << setw(4) << _ci << "\t" << ((o.op | o.arg) & 0x00ff ? (o.op | o.arg) : (o.op | o.arg) >> 8) << "\t" << dec << lineno + 1 << "\t" << line.label + "\t" + line.mnemonic + "\t" + line.arg + "\t" + line.comment << endl;
		list.push_back(str.str());
	}

	void do_create_obj(vector<obj> &obj_list, string client)
	{

		if (obj_list.empty())
			return;

		stringstream str, str_bin;
		str << hex << file_number;
		stringstream dir;
		dir << ".";
		if (client != "")
			dir << "/usr/" << client;

		dir << "/bin/" << file_name.substr(0, file_name.length() - 4) << str.str();
		stringstream hex_dir;
		stringstream bin_dir;
		hex_dir << dir.str() << ".o";
		bin_dir << dir.str() << ".bin";
		ofstream hex_file(hex_dir.str(), ofstream::out);
		ofstream bin_file(bin_dir.str(), ofstream::out | std::ios_base::binary);
		str.str("");

		str << hex << setfill('0') << setw(2) << ((initial_addr >> 8) & 0x00ff) << " " << hex << setfill('0') << setw(2) << (initial_addr & 0x00ff) << " " << hex << setfill('0') << setw(2) << (block_size & 0x00ff) << endl;
		str_bin << (char)(initial_addr >> 8) << (char)(initial_addr & 0x00ff) << (char)(block_size & 0x00ff);

		for (int i = 0; i < obj_list.size(); i++)
		{
			str << hex << setfill('0') << setw(2) << (((obj_list[i].op | obj_list[i].arg) & 0xff00) >> 8) << " ";

			str_bin << (char)(((obj_list[i].op | obj_list[i].arg) & 0xff00) >> 8);

			stringstream teste;
			teste << hex << setfill('0') << setw(2) << (((obj_list[i].op | obj_list[i].arg) & 0xff00) >> 8);

			if (teste.str().compare("0a") == 0)
				cout << "+++++++++++++++++++++++++++++++++++++++++++\n+++++++++++++++++++++++++++++++++++++++++++\n"
					 << hex << str_bin.hex << "\n+++++++++++++++++++++++++++++++++++++++++++\n"
					 << endl;

			if ((obj_list[i].arg & 0x00ff) != 0x00 || (mnemonic_table[obj_list[i].op >> 12].size == 2 && !obj_list[i].pseudo))
			{
				cout << "IF" << endl;
				str << hex << setfill('0') << setw(2) << (obj_list[i].arg & 0x00ff) << " ";
				str_bin << (char)(obj_list[i].arg & 0x00ff);
			}
			if (i != 0 && i % 4 == 0)
				str << endl;
		}

		obj_list.clear();
		block_size = 0;

		// CheckSum
		BYTE check_sum = 0x00;
		for (int i = 0; i < str_bin.str().length(); i++)
		{
			stringstream ss;
			ss << str_bin.str()[i];
			char cByte;
			ss.read(&cByte, 2);
			check_sum += (uint16_t)cByte & 0x00ff;
		}
		check_sum = check_sum & 0x00ff;
		check_sum = ~check_sum & 0x00ff;
		check_sum += 0x01;

		str_bin << (char)check_sum;
		str << endl
			<< hex << (WORD)check_sum << endl;

		bin_file << str_bin.rdbuf();

		hex_file << str.rdbuf();

		bin_file.close();
		hex_file.close();
		str.str("");
		str_bin.str("");
		file_number += 1;
	}

	assembled Assemble(string file, string client = "")
	{

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
		for (int step = 1; step < 3; step++)
		{
			_ci = 0;
			// Analizamos cada uma das linhas obtidas no processamento do arquivo
			for (int i = 0; i < lines.size(); i++)
			{

				if (lines[i].mnemonic == "")
				{ // Comment or blank line (will be ignored)
					if ((step == 2) && (lines[i].comment != ""))
					{										// Just a comment line
						do_list(list, lines[i].comment, i); // List comment line if step = 2
						continue;
					}
					continue;
				}

				// Analise do label
				if (step == 1)
				{
					if (lines[i].label != "")
					{ // Has label
						if (simbols_list.find(lines[i].label) == simbols_list.end())
						{ // Create new simbol
							stringstream str;
							str << "/" << hex << setfill('0') << setw(4) << _ci;
							simbols_list[lines[i].label] = str.str();
						}
						else if (simbols_list[lines[i].label] == "undefined")
						{ // Simbol undefined
							stringstream str;
							str << "/" << hex << setfill('0') << setw(4) << _ci;
							simbols_list[lines[i].label] = str.str();
						}
						else
						{
							//TODO: Better Errors...
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
				for (int j = 0; j < 0xd; j++)
				{
					if (mnemonic_table[j].name.find(lines[i].mnemonic) == 0)
					{
						op = mnemonic_table[j].op;
						size = mnemonic_table[j].size;
					}
				}

				// Se nao foi encontrado, pode ser uma pseudoinstrução
				if (op == -1)
				{
					if (lines[i].mnemonic == "@")
					{ // ORIGIN

						if (step == 2)
						{
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
					}
					else if (lines[i].mnemonic == "$")
					{ // ARRAY
						stringstream str;
						str << hex << lines[i].arg.substr(1, string::npos);
						int s_arr;
						str >> s_arr;
						s_arr &= 0x0fff;
						_ci += s_arr;
						if (step == 2)
							do_list(list, "\t" + lines[i].mnemonic + "\t" + lines[i].arg + "\t" + lines[i].comment, i);
						continue;
					}
					else if (lines[i].mnemonic == "K")
					{ // CONSTANT
						if (lines[i].label == "")
						{
							cout << "Assembler Error: Constant without label" << endl;
							exit(1);
						}
						stringstream str;

						str << hex << lines[i].arg.substr(1, string::npos);
						int k;
						str >> k;

						k &= 0x00ff;
						if (step == 2)
						{
							obj_list.push_back(obj((k << 8) & 0xf000, (k & 0x000f) << 8, true));
							do_list(list, lines[i], i, obj_list.back());
							block_size += 1;
							continue;
						}
						_ci += 1;
					}
					else if (lines[i].mnemonic == "#")
					{ // END
						if (step == 2)
						{
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
				if (lines[i].arg[0] == '/')
				{ // Is literal hex
					if (step == 2)
					{
						stringstream str;
						str << hex << lines[i].arg.substr(1, string::npos);
						int arg;
						str >> arg;
						if (size == 2)
							obj_list.push_back(obj(op << 4 * 3, arg & 0x0fff));
						else
							obj_list.push_back(obj(op << 4 * 3, (arg & 0x000f) << 8));

						do_list(list, lines[i], i, obj_list.back());
					}
				}
				else
				{ // Is label

					// Suporte para notação de array
					bool isArray = lines[i].arg.find("[") != string::npos && lines[i].arg.find("]") != string::npos;

					map<string, string>::iterator it = simbols_list.find(lines[i].arg);

					if ((step == 1) && (it == simbols_list.end()))
					{ // Undefined simbol
						simbols_list[lines[i].arg] = "undefined";
					}

					if (step == 2 && it != simbols_list.end())
					{ //Label already defined, generate obj code
						stringstream str;
						if (!isArray)
							str << hex << simbols_list[lines[i].arg].substr(1, string::npos);
						else
						{
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
						obj_list.push_back(obj(op << 4 * 3, arg & 0x0fff));
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

		cout << endl
			 << "LABELS"
			 << "\t"
			 << "VALUE" << endl;
		cout << "==========================" << endl;

		for (map<string, string>::iterator it = simbols_list.begin(); it != simbols_list.end(); ++it)
		{
			cout << it->first << "\t" << hex << it->second << endl;
		}

		cout << endl
			 << "ADDRES\t"
			 << "OBJECT\t"
			 << "LINE\t"
			 << "SOURCE\t" << endl;
		for (int i = 0; i < list.size(); i++)
		{
			cout << list[i] << endl;
		}

		return assembled(file_number, begin_addr, file_name);
	}
};
