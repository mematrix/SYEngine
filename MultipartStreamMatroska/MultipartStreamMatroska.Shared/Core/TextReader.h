#ifndef __TEXT_READER_H
#define __TEXT_READER_H

class StringArray;
class TextReader
{
public:
	struct TextLine
	{
		char** Line;
		int Count;
	};

	TextReader() throw();
	explicit TextReader(int max_count) throw();
	~TextReader() throw();

	bool ParseFile(const char* file_path) throw();
	bool ParseFile(const wchar_t* file_path) throw(); //_GLIBCXX_USE_WCHAR_T

	inline TextLine* GetTextLine() throw() { return &_lines; }

private:
	void ReadLines(void* fs, bool wchar);

	TextLine _lines;
	int _max_count;

	StringArray* _dyn_lines;
};

#endif //__TEXT_READER_H