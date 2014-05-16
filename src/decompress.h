/*
 * FILE             $Id: decompress.h 13 2008-11-14 07:55:30Z burlog $
 *
 * PROJECT          KHTML JavaScript compress utility
 *
 * DESCRIPTION      DeCompress string dump
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

#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <sstream>
#include <string>

namespace KJS { class Identifier; class Node;}

/**
 * @short DeCompress stream - used for decompress javascript source.
 */
class DeCompressStream_t {
public:
    enum Format_t { ENDL, INDENT, UNINDENT, };

    /**
     * @short dump node tree to stream and return stream.
     * @param node node tree to dump.
     */
    DeCompressStream_t(const KJS::Node *node);

    /**
     * @short return decompressed javascript source.
     * @return decompressed javascript source.
     */
    std::string string() const { return os.str();}

    /**
     * @short dump node to stream and return stream.
     * @param node to dump.
     * @return compressed stream.
     */
    friend DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                                          const KJS::Node *node);
    friend DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                                          const KJS::Identifier &value);
    friend DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                                          const std::string &value);
    friend DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                                          const char *value);
    friend DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                                          const char &value);
    friend DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                                          DeCompressStream_t::Format_t value);

private:
    std::ostringstream os; //< stream buffer for javascript source
    std::string indent;    //< indentation buffer
};

#endif /* DECOMPRESS_H */

