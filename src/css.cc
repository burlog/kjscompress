/*
 * FILE             $Id: css.cc 21 2011-03-04 08:28:25Z burlog $
 *
 * PROJECT          KHTML JavaScript compress utility
 *
 * DESCRIPTION      CSS Comprimator
 *
 * AUTHOR           Josef Sima <josef.sima@firma.seznam.cz>
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * LICENSE          see COPYING
 *
 * Copyright (C) Seznam.cz a.s. 2006
 * All Rights Reserved
 *
 * HISTORY
 *      2006-07-28 (sima)
 *              First draft
 *
 *      2006-08-01 (sima)
 *              CSS comprimator completed, some bugfixs
 *
 *       2007-09-26 (bukovsky)
 *              Imported to svn.
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>
#include <ctype.h>
#include <string>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <cstdlib>
#include <cstdio>

// definice stavu automatu
#define CSS_S_NORMAL        0
#define CSS_S_COMMENTSWH    1
#define CSS_S_COMMENTSTD    2
#define CSS_S_COMMENTDEL    3
#define CSS_S_SPACES        4
#define CSS_S_QUOTE         5
#define CSS_S_DBLQUOTE      6
#define CSS_S_SEMICOLON     7
#define CSS_S_NOWHITESAFTER 8

#define OPTIONS "hf:t:"
#define USAGE "Usage: csscompress [Options]\n\
    -h        show this help\n\
    -f file   read css code from file\n\
    -t file   dump css code to file\n\
    \n\
    CSS compress utility\n\
    Josef Sima <josef.sima@firma.seznam.cz>\n\
    Copyright (C) Seznam.cz a.s. 2007"

class SpaceShrinker_t {
public:
    SpaceShrinker_t(std::istream &stream)
        : stream(stream), flushed(true), ch(0)
    {}

    SpaceShrinker_t &get(char &rch) {
        if (!flushed) {
            flushed = true;
            rch = ch;
            return *this;
        }

        bool spaces = false;
        while (stream.get(rch)) {
            if (rch != ' ')
                break;
            spaces = true;
        }
        if (spaces) {
            stream.unget();
            rch = ' ';
        }
        ch = rch;
        return *this;
    }

    void unget() {
        flushed = false;
    }

    operator bool() {
        return !stream.eof();
    }

private:
    std::istream &stream;
    bool flushed;
    char ch;
};

/**
 * @short Main method.
 * @param argc arguments count.
 * @param *argv arguments array.
 * @return return code.
 */
int main(int argc, char **argv) {
    // options
    std::string from;
    std::string to;

    /* getopt unistd.h */
    char options = 0;
    bool error_opt = false;
    struct stat st;

    // prase options
    while ((options = getopt(argc, argv, OPTIONS)) != EOF) {
        switch (options) {
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
        case 'h':
        default:
            error_opt = true;
            break;
        }
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

    {
        // css parser
        char parserState = CSS_S_NORMAL;
        char actualChar, nextChar = ' ';
        SpaceShrinker_t input(in);
        while (input.get(actualChar)) {

            switch (parserState) {

                case CSS_S_NORMAL:
                    //std::cout << "CSS_S_NORMAL" << std::endl;

                    if (actualChar == '/') {
                        input.get(nextChar);
                        if (nextChar == '*') {
                            parserState = CSS_S_COMMENTSWH;
                        } else {
                            out << actualChar;
                            input.unget();
                        }
                    } else if (actualChar == '"') {
                        out << actualChar;
                        parserState = CSS_S_DBLQUOTE;
                    } else if (actualChar == '\'') {
                        out << actualChar;
                        parserState = CSS_S_QUOTE;
                    } else if (actualChar == ';') {
                        parserState = CSS_S_SEMICOLON;
                    } else if (actualChar == ':') {
                        out << actualChar;
                        input.get(nextChar);
                        if (isspace(actualChar)) {
                            out << ' ';
                            parserState = CSS_S_NOWHITESAFTER;
                        } else {
                            input.unget();
                        }
                    } else if ((actualChar == ',') || (actualChar == '{')
                               || (actualChar == '}')) {
                        out << actualChar;
                        parserState = CSS_S_NOWHITESAFTER;
                    } else if (isspace(actualChar)) {
                        input.unget();
                        parserState = CSS_S_SPACES;
                    } else {
                        out << actualChar;
                    }
                break;

                case CSS_S_COMMENTSWH:
                    //std::cout << "CSS_S_COMMENTSWH" << std::endl;

                    if (actualChar == '-') {
                        parserState = CSS_S_COMMENTDEL;
                    } else {
                        parserState = CSS_S_COMMENTSTD;
                        out << "/*";
                        input.unget();
                    }
                    break;

                case CSS_S_COMMENTSTD:
                    //std::cout << "CSS_S_COMMENTSTD" << std::endl;

                    if (actualChar == '*') {
                        input.get(nextChar);
                        if (nextChar == '/') {
                            parserState = CSS_S_NORMAL;
                            out << actualChar << nextChar;
                        } else {
                            out << actualChar;
                            input.unget();
                        }
                    } else if (!iscntrl(actualChar)) {
                        out << actualChar;
                    }
                break;

                case CSS_S_COMMENTDEL:
                    //std::cout << "CSS_S_COMMENTDEL" << std::endl;

                    if (actualChar == '*') {
                        input.get(nextChar);
                        if (nextChar == '/') {
                            parserState = CSS_S_NORMAL;
                        } else {
                            input.unget();
                        }
                    }
                break;

                case CSS_S_SEMICOLON:
                    //std::cout << "CSS_S_SEMICOLON" << std::endl;

                    if (!isspace(actualChar)) {
                        if (actualChar == '}') {
                            input.unget();
                        } else {
                            input.unget();
                            out << ';';
                        }
                        parserState = CSS_S_NORMAL;
                    }
                break;

                case CSS_S_NOWHITESAFTER:
                    //std::cout << "CSS_S_NOWHITESAFTER" << std::endl;

                    if (!isspace(actualChar)) {
                        input.unget();
                        parserState = CSS_S_NORMAL;
                    }
                break;

                case CSS_S_DBLQUOTE:
                    //std::cout << "CSS_S_DBLQUOTE" << std::endl;

                    if (actualChar == '\\') {
                        input.get(nextChar);
                        if (nextChar == '"') {
                            out << actualChar << nextChar;
                        } else if (nextChar == '\\') {
                            out << actualChar << nextChar;
                        } else {
                            out << actualChar;
                            input.unget();
                        }
                    } else if (actualChar == '"') {
                        out << actualChar;
                        parserState = CSS_S_NORMAL;
                    } else if (!iscntrl(actualChar)) {
                        out << actualChar;
                    }
                break;

                case CSS_S_QUOTE:
                    //std::cout << "CSS_S_QUOTE" << std::endl;

                    if (actualChar == '\\') {
                        input.get(nextChar);
                        if (nextChar == '\'') {
                            out << actualChar << nextChar;
                        } else if (nextChar == '\\') {
                            out << actualChar << nextChar;
                        } else {
                            out << actualChar;
                            input.unget();
                        }
                    } else if (actualChar == '\'') {
                        out << actualChar;
                        parserState = CSS_S_NORMAL;
                    } else if (!iscntrl(actualChar)) {
                        out << actualChar;
                    }
                break;

                case CSS_S_SPACES:
                    //std::cout << "CSS_S_SPACES" << std::endl;

                    input.get(nextChar);
                    if (nextChar == '{' || nextChar == '}') {
                        input.unget();
                    } else if (!(isspace(nextChar))) {
                        out << ' ';
                        input.unget();
                    }
                    parserState = CSS_S_NORMAL;
                break;
            }

        }
    }

    // return succes
    return EXIT_SUCCESS;
}

