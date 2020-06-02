#include "tokenizer.hpp"

#include <memory>
#include <stdio.h>
#include <string>

void Tokenizer::getLine()
{
    // -- Read a line
    m_num_tokens = 0;
    string std::unique_ptr<string> line = 
    // TODO: char* res = fgets(m_line, LINESIZE, m_file);


    if (ifstream.eof()) {
        this->m_done = true;
        return;
    } else { 
        m_cur_line++;
        // -- Break line up into tokens
        // 1. Split on comma,
        // 2. Then assign to m_tokens as a unique_ptr
        m_line_saved = std::move(m_line);
    }

}

unsigned int Tokenizer::getInt(int i)
{
    if (i < m_num_tokens) {
        unsigned int value = 0;
        sscanf(m_tokens[i], "%d", &value);
        return value;
    } else {
        return 0;
    }
}

char* Tokenizer::getString(int i)
{
    if (i < m_num_tokens) {
        return m_tokens[i];
    }
    else {
        return 0;
    }
}

char Tokenizer::getChar(int i)
{
    if (i < m_num_tokens) {
        return m_tokens[i][0];
    }
    else {
        return 0;
    }
}

void Tokenizer::debugCurrent()
{
    cout << "DEBUG: current line = " << m_cur_line << endl;
    string tmp(m_line_saved);
    cout << "  ---> " << tmp << endl;

}
