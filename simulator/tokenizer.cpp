#include "tokenizer.hpp"

#include <stdio.h>
#include <string.h>

void Tokenizer::getLine()
{
    // -- Read a line
    m_num_tokens = 0;
    char* res = fgets(m_line, LINESIZE, m_file);
    if (res) {
        m_cur_line++;
        m_line_saved = m_line;
        // -- Break line up into tokens
        ///   Basically, just replace spaces with \0 and remember the start of each token

        // -- Set to true so that the first char is identified as a token
        int cur = 0;
        bool prev_was_space = true;
        while ( (m_line[cur] != '\0') &&
                (cur < LINESIZE) ) {
            if (isspace(m_line[cur])) {
                // -- Replace spaces with \0
                prev_was_space = true;
                m_line[cur] = '\0';
            } else {
                // -- Not a space (part of a token)
                if (prev_was_space) {
                    // -- Found the start of t a token
                    m_tokens[m_num_tokens] = &(m_line[cur]);
                    m_num_tokens++;
                }
                prev_was_space = false;
            }
            cur++;
        }      
    }

    m_done = (m_num_tokens == 0);
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
