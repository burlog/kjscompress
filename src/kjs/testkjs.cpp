/*
 * FILE             $Id: testkjs.cpp 5088 2008-05-12 10:52:15Z michal.bukovsky $
 *
 * DESCRIPTION      kjs compress utility
 *
 * PROJECT          kjs compress utility
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2006
 * All Rights Reserved
 *
 * HISTORY
 *       2007-02-26 (bukovsky)
 *                       Frist draft
 */

#include <string>
#include <iostream>
#include <sstream>
#include <iterator>
#include <iomanip>

#include "nodes.h"
#include "ustring.h"

namespace KJS {
    /**
     * A simple text streaming class that helps with code indentation.
     */
    class SourceStream {
    public:
        enum Format {
            Endl, Indent, Unindent
        };

        UString toString() const { return str; }
        SourceStream& operator<<(const Identifier &);
        SourceStream& operator<<(const KJS::UString &);
        SourceStream& operator<<(const char *);
        SourceStream& operator<<(char);
        SourceStream& operator<<(Format f);
        SourceStream& operator<<(const Node *);
    private:
        UString str; /* TODO: buffer */
        UString ind;
    };
}

int main(int argc, char **argv) {
    bool res = true;

    std::ostringstream os;
    std::cin >> std::noskipws;
    std::copy(std::istream_iterator<char>(std::cin),
            std::istream_iterator<char>(),
            std::ostream_iterator<char>(os));

    // parse
    KJS::UString code(os.str().c_str());
    int errLine = -1;
    KJS::UString errMsg;
    KJS::SourceCode *source = 0;
    KJS::FunctionBodyNode *progNode = KJS::Parser::parse(
            code.data(), code.size(), &source, &errLine, &errMsg);

    // compress
    KJS::SourceStream stream;
    progNode->streamTo(stream);
    std::cout << stream.toString().ascii() << std::endl;

    // validate
    {
        KJS::UString code(stream.toString());
        int errLine = -1;
        KJS::UString errMsg;
        KJS::SourceCode *source = 0;
        KJS::FunctionBodyNode *progNode = KJS::Parser::parse(
                code.data(), code.size(), &source, &errLine, &errMsg);

        if (errLine >= 0)
            std::cerr << "Error: " << errLine << ": " << errMsg.ascii() << std::endl;
    }

    return res;
}
