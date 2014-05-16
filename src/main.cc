/*
 * FILE             $Id: main.cc 9 2008-05-30 14:56:21Z burlog $
 *
 * PROJECT          KHTML JavaScript compress utility
 *
 * DESCRIPTION      Compress utility main file
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * LICENSE          see COPYING
 *
 * Copyright (C) Seznam.cz a.s. 2007
 * All Rights Reserved
 *
 * HISTORY
 *       2007-02-26 (bukovsky)
 *                       Frist draft
 */

#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ostream>
#include <iterator>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "decompress.h"
#include "compress.h"
#include "kjs/nodes.h"

#define CODE_DUMP_LEN 30

#define OPTIONS "hnve:dob:cB:p:af:t:"
#define USAGE "Usage: ksjcompress [Options]\n\
    -h        show this help\n\
    -f file   read js code from file\n\
    -t file   dump js code to file\n\
    -d        decompress code\n\
    -v        not valide generated code\n\
    -n        add new lines to compressed code\n\
    -e x      dump x chars brefore and after error [30]\n\
    -o        obfuscate identfiers\n\
    -c        write origin identfier in comment\n\
    -a        ask to user whether obfuscate identfier\n\
    -p prefix dont obfuscate identfiers with prefix\n\
    -b file   identfiers obfuscate blacklist\n\
    -B file   dump blacklist after obfuscate to file\n\n\
    KHTML JavaScript compress utility\n\
    Michal Bukovsky <michal.bukovsky@firma.seznam.cz>\n\
    Copyright (C) Seznam.cz a.s. 2007"

/**
 * @short main fuction
 */
int main(int argc, char **argv) {
    // options
    bool eof = false;
    bool compress = true;
    bool validate = true;
    bool obfuscate = false;
    bool comment = false;
    bool ask = false;
    std::string prefix;
    std::string blacklist;
    std::string blacklistDump;
    std::string from;
    std::string to;
    int code_dump_len = CODE_DUMP_LEN;

    /* getopt unistd.h */
    char options = 0;
    bool error_opt = false;
    struct stat st;

    // error handling
    int errLine = -1;
    int errChar = -1;
    KJS::UString errMsg;
    KJS::SourceCode *source = 0;

    // prase options
    while ((options = getopt(argc, argv, OPTIONS)) != EOF) {
        switch (options) {
        case 'd':
            compress = false;
            break;
        case 'n':
            eof = true;
            break;
        case 'v':
            validate = false;
            break;
        case 'o':
            obfuscate = true;
            break;
        case 'c':
            comment = true;
            break;
        case 'a':
            ask = true;
            break;
        case 'p':
            prefix = optarg;
            break;
        case 'B':
            blacklistDump = optarg;
            break;
        case 't':
            to = optarg;
            break;
        case 'f':
            from = optarg;
            if ((from != "-") && stat(optarg, &st)) {
                std::cerr << "Cannot open file: " << optarg << "." << std::endl;
                std::cerr << strerror(errno) << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case 'b':
            blacklist = optarg;
            if (stat(optarg, &st)) {
                std::cerr << "Cannot open file: " << optarg << "." << std::endl;
                std::cerr << strerror(errno) << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case 'e':
            code_dump_len = atoi(optarg);
            if (code_dump_len < 0) {
                std::cerr << "Ignore not valid param for -e option."
                    << std::endl;
                code_dump_len = CODE_DUMP_LEN;
            }
            break;
        case 'h':
        default:
            error_opt = true;
            break;
        }
    }

    if (ask && from.empty()) {
        std::cerr << "Ignore -a option. Can be used only with -f option."
            << std::endl;
        ask = false;
    }

    // option parse error?
    if (error_opt || (optind < argc)) {
        std::cerr << USAGE << std::endl;
        return EXIT_FAILURE;
    }

    // open input/output
    std::ifstream in;
    if ((from == "-") || (from.empty())) {
        in.copyfmt(std::cin);
        in.clear(std::cin.rdstate());
        in.std::basic_ios<char>::rdbuf(std::cin.rdbuf());
    } else {
        in.open(from.c_str());
    }
    if (!in) {
        std::cerr << "Cannot open input file." << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }
    std::ofstream out;
    if ((to == "-") || (to.empty())) {
        out.copyfmt(std::cout);
        out.clear(std::cout.rdstate());
        out.std::basic_ios<char>::rdbuf(std::cout.rdbuf());
    } else {
        out.open(to.c_str());
    }
    if (!in) {
        std::cerr << "Cannot open output file." << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    // read input
    std::ostringstream os;
    in >> std::noskipws;
    std::copy(std::istream_iterator<char>(in),
            std::istream_iterator<char>(),
            std::ostream_iterator<char>(os));

    // parse via kjs
    std::string theCode = os.str();
    KJS::UString code(theCode.c_str());
    KJS::FunctionBodyNode *node = KJS::Parser::parse(code.data(), code.size(),
            &source, &errLine, &errChar, &errMsg);

    // report error
    if (errLine >= 0) {
        std::cerr << "ERR: " << errMsg.ascii() << std::endl;

        // dump some bit of code
        int from = ((errChar - code_dump_len) > 0)?
            errChar - code_dump_len: 0;
        int newLine = 0;
        for (int i = from; (i < errChar) || ((i < (signed)theCode.size())
                    && (theCode[i] != '\n')); ++i) {
            std::cerr << theCode[i];
            if (i < errChar) {
                if (theCode[i] == '\n')
                    newLine = 0;
                else
                    ++newLine;
            }
        }
        std::cerr << std::endl;

        // dump error mark
        for (int i = 0; i < newLine; ++i)
            std::cerr << ' ';
        std::cerr << '^' << std::endl;

        return EXIT_FAILURE;
    }

    // transform
    std::string transformed;
    if (compress)
        transformed = CompressStream_t(node, obfuscate, comment, ask, prefix,
                blacklist, blacklistDump, eof).string();
    else
        transformed = DeCompressStream_t(node).string();

    // validate compressed
    if (validate) {
        KJS::UString code(transformed.c_str());
        KJS::SourceCode *source = 0;
        KJS::Parser::parse(code.data(), code.size(), &source, &errLine,
                &errChar, &errMsg);

        // report error
        if (errLine >= 0) {
            std::cerr << "VALIDATE_ERR: " << errMsg.ascii() << std::endl;

            // dump some bit of compressed code
            int from = ((errChar - code_dump_len) > 0)?
                    errChar - code_dump_len: 0;
            int to = ((errChar + code_dump_len) < (signed)transformed.size())?
                    (errChar + code_dump_len): transformed.size();
            int newLine = 0;
            for (int i = from; i < to; ++i) {
                if ((i >= errChar) && (transformed[i] == '\n'))
                    break;
                std::cerr << transformed[i];
                if (transformed[i] == '\n')
                    newLine = 0;
                else
                    ++newLine;
            }
            std::cerr << std::endl;

            // dump error mark
            for (int i = from; i < ((errChar < newLine)? errChar: newLine); ++i)
                std::cerr << ' ';
            std::cerr << '^' << std::endl;

            // error
            return EXIT_FAILURE;
        }
    }

    // write transformed code
    out << transformed << std::endl;

    // return succes
    return EXIT_SUCCESS;
}

