/*
 * FILE             $Id: util.cc 22 2011-04-06 10:13:55Z burlog $
 *
 * PROJECT          KHTML JavaScript compress utility
 *
 * DESCRIPTION      Compress string dump
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * LICENSE          see COPYING
 *
 * Copyright (C) Seznam.cz a.s. 2007
 * All Rights Reserved
 *
 * HISTORY
 *       2007-03-09 (bukovsky)
 *                  First draft.
 */

#include "util.h"

/**
 * @short escape string.
 * @param str string to escape.
 * @return escaped string.
 */
std::string escape(const std::string &str) {
    std::string escaped;
    escaped.reserve(str.size());

    // iterate over all string chars
    for (std::string::size_type i = 0; i < str.size(); i++) {

        switch (str[i]) {
        case '\n':
            escaped.append("\\n");
            break;
        case '\r':
            escaped.append("\\r");
            break;
        case '\\':
            // do not escape backslah for unicode seq
            if (((i + 5) < str.size())
                && (str[i + 1] == 'u')
                && isxdigit(str[i + 2])
                && isxdigit(str[i + 3])
                && isxdigit(str[i + 4])
                && isxdigit(str[i + 5]))
            {
                escaped.append("\\");

            // do not escape backslah for hexa seq
            } else if (((i + 3) < str.size())
                && (str[i + 1] == 'x')
                && isxdigit(str[i + 2])
                && isxdigit(str[i + 3]))
            {
                escaped.append("\\");

            // do not escape backslah for octal seq
            } else if (((i + 3) < str.size())
                && (str[i + 1] >= '0' && str[i + 1] <= '3')
                && (str[i + 2] >= '0' && str[i + 2] <= '7')
                && (str[i + 3] >= '0' && str[i + 3] <= '7'))
            {
                escaped.append("\\");

            } else if (((i + 3) < str.size())
                && (str[i + 1] >= '0' && str[i + 1] <= '7')
                && (str[i + 2] >= '0' && str[i + 2] <= '7'))
            {
                escaped.append("\\");

            } else if (((i + 3) < str.size())
                && (str[i + 1] >= '0' && str[i + 1] <= '7'))
            {
                escaped.append("\\");

            // single backslah escape it
            } else {
                escaped.append("\\\\");
            }
            break;
        case '\b':
            escaped.append("\\b");
            break;
        case '\"':
            escaped.append("\\\"");
            break;
        case '\t':
            escaped.append("\\t");
            break;
        default:
            escaped.append(1, str[i]);
            break;
        }
    }
    return escaped;
}

