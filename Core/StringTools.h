#pragma once

// Create a formatted text string
string StringFormat(const char *inFMT, ...);

// Convert type to string
template<typename T>
string ConvertToString(const T &inValue)
{
    ostringstream oss;
    oss << inValue;
    return oss.str();
}

// Replace substring with other string
void StringReplace(string &ioString, string inSearch, string inReplace);

// Trim characters from left side of string
void TrimLeft(string &ioString, const char *inCharacters = " \t\r\n");

// Trim characters from right side of string
void TrimRight(string &ioString, const char *inCharacters = " \t\r\n");

// Trim characters from both sides of string
void Trim(string &ioString, const char *inCharacters = " \t\r\n");

// Convert a delimited string to an array of strings
void StringToVector(const string &inString, vector<string> &outVector, const string &inDelimiter = ",", bool inClearVector = true);

// Convert an array strings to a delimited string
void VectorToString(const vector<string> &inVector, string &outString, const string &inDelimiter = ",");

// Convert a string to lower case
string ToLower(const string &inString);

// Converts the lower 4 bits of inNibble to a string that represents the number in binary format
const char *NibbleToBinary(uint32 inNibble);
