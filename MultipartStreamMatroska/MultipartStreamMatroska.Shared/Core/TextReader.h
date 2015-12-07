#ifndef __TEXT_READER_H
#define __TEXT_READER_H

class TextReader
{
public:
	struct TextLine
	{
		char** Line;
		int Count;
	};

	explicit TextReader(int max_count = 128) throw();
	~TextReader() throw();

	bool ParseFile(const char* file_path) throw();
	bool ParseFile(const wchar_t* file_path) throw(); //_GLIBCXX_USE_WCHAR_T

	inline TextLine* GetTextLine() throw() { return &_lines; }

private:
	void ReadLines(void* fs, bool wchar);

	TextLine _lines;
	int _max_count;
};

#endif //__TEXT_READER_H