#include <pch.h> // IWYU pragma: keep
#include <fstream>

void FatalError(const char *inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsprintf(buffer, inFMT, list);

	// Throw exception
	throw string(buffer);
}

// Read file contents
vector<uint8> ReadData(const char *inFileName)
{
	vector<uint8> data;
	ifstream input(inFileName, std::ios::binary);
	if (!input)
		FatalError("Unable to open file: %s", inFileName);
	input.seekg(0, ios_base::end);
	ifstream::pos_type length = input.tellg();
	input.seekg(0, ios_base::beg);
	data.resize(length);
	input.read((char *)&data[0], length);
	if (!input)
		FatalError("Unable to read file: %s", inFileName);
	return data;
}
