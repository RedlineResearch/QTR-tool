#ifndef TOKENIZER_H
#define TOKENIZER_H

// ----------------------------------------------------------------------
//   Tokenizer
//   Reads a file line-by-line, breaking each line into tokens
//   which can be retrieved by type

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

// -- Size of temporary char arrays (yes, I know, this could cause buffer overflow)
#define LINESIZE 2048
#define TOKENSIZE 50

enum class DELIMITER_TYPE : signed char
{
    E_SPACE = 1,
    E_COMMA = 2,
    E_INVALID = 127
};

class Tokenizer
{
    private:
        FILE* m_file;
        char m_line[LINESIZE];
        string m_line_saved;
        char *m_tokens[TOKENSIZE];
        unsigned int m_num_tokens;
        bool m_done;
        unsigned int m_cur_line; // 1 based line counting
        DELIMITER_TYPE m_delimiter;
        bool m_debug;

    public:
        Tokenizer(FILE* f, DELIMITER_TYPE delimiter)
            : m_file(f)
            , m_done(false)
            , m_cur_line(0)
            , m_delimiter(delimiter)
            , m_debug(false) {
        }

        // -- Get the next line, break up into tokens
        void getLine();

        // -- Returns true if getLine produces no tokens
        bool isDone() const { return m_done; }

        // -- Get the number of tokens in the current line
        unsigned numTokens() const { return m_num_tokens; }

        // -- Get token #i as an int
        unsigned int getInt(int i);

        // -- Get token #i as a string
        char* getString(int i);

        // -- Get the first char of token #i
        char getChar(int i);

        // -- Print out current line and linenumber
        void debugCurrent();

        // -- Is separator?
        bool isDelimiter(char c) const {
            if (this->m_delimiter == DELIMITER_TYPE::E_SPACE) {
                return isspace(c);
            } else if (this->m_delimiter == DELIMITER_TYPE::E_COMMA) {
                return (c == ',');
            } else {
                return false;
            }
        }

        void setDebug(bool flag) { this->m_debug = flag; }
};

#endif
