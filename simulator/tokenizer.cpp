#include "tokenizer.hpp"

#include <memory>
#include <stdio.h>
#include <string>

void Tokenizer::getLine()
{
    // -- Read a line
    m_num_tokens = 0;

    if (ifstream.eof()) {
        this->m_done = true;
        return;
    } else { 
        string line;
        string token;
        std::getline(input, line);
        m_line_saved = line; // save to copy
        m_cur_line++;
        // -- Break line up into tokens
        // 1. Split on comma,
        // 2. Then assign to m_tokens as a unique_ptr
        // 3. Repeat if necessaryj
        size_t pos = 0;
        int cur_token = 0;
        while ((pos = line.find(',')) != std::string::npos) {
            token = line.substr(0, pos);
            m_tokens[cur_token] = token;
            line.erase(0, pos + 1);
            cur_token += 1;
        }
        // Last token
        token[cur_token] = line;
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
