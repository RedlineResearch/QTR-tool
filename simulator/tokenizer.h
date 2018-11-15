#ifndef TOKENIZER_H
#define TOKENIZER_H

// ----------------------------------------------------------------------
//   Tokenizer
//   Reads a file line-by-line, breaking each line into tokens
//   which can be retrieved by type

#include <iostream>

using namespace std;

// -- Size of temporary char arrays (yes, I know, this could cause buffer overflow)
#define LINESIZE 2048
#define TOKENSIZE 50

class Tokenizer
{
    private:
        FILE* m_file;
        char m_line[LINESIZE];
        string m_line_saved;
        char* m_tokens[TOKENSIZE];
        unsigned int m_num_tokens;
        bool m_done;
        unsigned int m_cur_line; // 1 based line counting

    public:
        Tokenizer(FILE* f)
            : m_file(f)
            , m_done(false)
            , m_cur_line(0) {
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
};

#endif
