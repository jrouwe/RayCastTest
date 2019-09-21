#pragma once

// Read file contents into byte vector
vector<uint8> ReadData(const char *inFileName);

// Output a line of text to the log / TTY.
#define Trace printf

// Display an error and exit.
void FatalError(const char *inFMT, ...);
