#include <pch.h> // IWYU pragma: keep

#include <Core/StringTools.h>
#include <cstdarg>

// Create a formatted text string
string StringFormat(const char *inFMT, ...)
{
	static char buffer[1024];

	// Format the string
	va_list list;
	va_start(list, inFMT);
	vsprintf(buffer, inFMT, list);

	return string(buffer);
}

// Replace substring with other string
void StringReplace(string &ioString, string inSearch, string inReplace)
{
	size_t index = 0;
	for (;;)
	{
		 index = ioString.find(inSearch, index);
		 if (index == std::string::npos) 
			 break;

		 ioString.replace(index, inSearch.size(), inReplace);

		 index += inReplace.size();
	}
}

// Trim characters from left side of string
void TrimLeft(string &ioString, const char *inCharacters)
{
	while (ioString.length() > 0 && strchr(inCharacters, ioString[0]) != nullptr)
		ioString.erase(0, 1);
}

// Trim characters from right side of string
void TrimRight(string &ioString, const char *inCharacters)
{
	while (ioString.length() > 0 && strchr(inCharacters, ioString[ioString.length() - 1]) != nullptr)
		ioString.erase(ioString.length() - 1, 1);
}

// Trim characters from both sides of string
void Trim(string &ioString, const char *inCharacters)
{
	TrimLeft(ioString, inCharacters);
	TrimRight(ioString, inCharacters);
}

// Convert a delimited string to an array of strings
void StringToVector(const string &inString, vector<string> &outVector, const string &inDelimiter, bool inClearVector)
{
	assert(inDelimiter.size() > 0);

	// Ensure vector empty
	if (inClearVector)
		outVector.clear(); 

	// No string? no elements
	if (inString.empty())
		return;

	// Start with initial string
	string s(inString);

	// Add to vector while we have a delimiter
	size_t i;
	while (!s.empty() && (i = s.find(inDelimiter)) != string::npos)
	{
		outVector.push_back(s.substr(0, i));
		s.erase(0, i + inDelimiter.length());
	}

	// Add final element
	outVector.push_back(s);
}

// Convert an array strings to a delimited string
void VectorToString(const vector<string> &inVector, string &outString, const string &inDelimiter)
{
	// Ensure string empty
	outString.clear();

	for (vector<string>::const_iterator i = inVector.begin(); i != inVector.end(); ++i)
	{
		// Add delimiter if not first element
		if (!outString.empty())
			outString.append(inDelimiter);

		// Add element
		outString.append(*i);
	}
}

// Convert a string to lower case
string ToLower(const string &inString)
{
	string out;
	out.reserve(inString.length());
	for (char c : inString)
		out.push_back((char)tolower(c));
	return out;
}

// Converts the lower 4 bits of inNibble to a string that represents the number in binary format
const char *NibbleToBinary(uint32 inNibble)
{
	static const char *nibbles[] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111" };
	return nibbles[inNibble & 0xf];
}
