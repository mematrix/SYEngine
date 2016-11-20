#include "TextReader.h"
#include "StringArray.h"
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>

TextReader::TextReader() throw() : _max_count(0)
{
	_lines.Line = NULL;
	_lines.Count = 0;
	_dyn_lines = new StringArray();
}

TextReader::TextReader(int max_count) throw() : _max_count(max_count)
{
	_lines.Line = (char**)malloc((max_count + 1) * sizeof(char*));
	_lines.Count = 0;
	_dyn_lines = NULL;
}

TextReader::~TextReader() throw()
{
	if (_dyn_lines == NULL && _lines.Line != NULL) {
		for (int i = 0; i < _lines.Count; ++i) {
			if (_lines.Line[i] != NULL)
				free(_lines.Line[i]);
		}
		free(_lines.Line);
	}
	if (_dyn_lines)
		delete _dyn_lines;
}

bool TextReader::ParseFile(const char* file_path) throw()
{
	std::ifstream file(file_path);
	if (!file)
		return false;
	ReadLines(&file, false);
	return _lines.Count > 0;
}

bool TextReader::ParseFile(const wchar_t* file_path) throw()
{
	std::ifstream file(file_path);
	if (!file)
		return false;
	ReadLines(&file, true);
	return _lines.Count > 0;
}

void TextReader::ReadLines(void* fs, bool wchar)
{
	std::ifstream* file = (std::ifstream*)fs;
	std::string line;
	while (std::getline(*file, line))
	{
		if (_lines.Count >= _max_count && _max_count > 0)
			break;
		if (_dyn_lines == NULL) {
			_lines.Line[_lines.Count] = (char*)malloc((line.length() + 1) * (wchar ? 2 : 1));
			if (_lines.Line[_lines.Count] == NULL)
				break;
			strcpy(_lines.Line[_lines.Count], line.c_str());
		}else{
			if (!_dyn_lines->push_back(line.c_str()))
				break;
		}
		++_lines.Count;
	}
	if (_dyn_lines != NULL)
		_lines.Line = _dyn_lines->getArrayPointer();
}