/*
 * FILE             $Id: compress.h 13 2008-11-14 07:55:30Z burlog $
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

#ifndef COMPRESS_H
#define COMPRESS_H

#include <sstream>
#include <string>
#include <map>
#include <set>

namespace KJS { class Identifier; class Node;}

/**
 * @short simple base62 integer
 */
class Base62Int_t {
public:
    /**
     * @short create new base62 number
     * @param i create with this value
     */
    Base62Int_t(unsigned int i = 0): i(i) {}

    /**
     * @short int increment operator
     * @return copy of this
     */
    Base62Int_t operator++() { ++i; return *this;}

    /**
     * @short dump base62 int to string digit
     * @return string digit
     */
    std::string str() const;

private:
    /**
     * @short convert int to base62 digit
     * @param x int from <0, 62>
     * @return char represent base62 digit
     */
    char toDigit(unsigned int x) const;

    unsigned int i;    //< integer value
};

/**
 * @short Compress stream - used for compress javascript source.
 */
class CompressStream_t {
public:
    typedef std::map<std::string, std::string> IdentifierMap_t;
    typedef std::set<std::string> StringSet_t;
    enum Format_t { ENDL};

    /**
     * @short dump node tree to stream and return stream.
     * @param node node tree to dump.
     * @param obfuscate do obfuscate.
     * @param comment write origin Identifier in comment.
     * @param ask ask user whether obfuscate identifier.
     * @param prefix dont obfuscate identfiers with prefix
     * @param blacklistDump dump blacklist to this file.
     * @param endl  write endl to buffer.
     */
    CompressStream_t(const KJS::Node *node,
            bool obfuscate = false, bool comment = false, bool ask = false,
            const std::string &prefix = std::string(),
            const std::string &userBlacklist = std::string(),
            const std::string &blacklistDump = std::string(),
            bool endl = false);

    /**
     * @short dump obfuscate log and destroy object.
     */
    ~CompressStream_t();

    /**
     * @short return compressed javascript source.
     * @return compressed javascript source.
     */
    std::string string() const { return os.str();}

    /**
     * @short dump node to stream and return stream.
     * @param node to dump.
     * @return compressed stream.
     */
    friend CompressStream_t &operator<<(CompressStream_t &cs,
                                        const KJS::Node *node);
    friend CompressStream_t &operator<<(CompressStream_t &cs,
                                        const KJS::Identifier &value);
    friend CompressStream_t &operator<<(CompressStream_t &cs,
                                        const std::string &value);
    friend CompressStream_t &operator<<(CompressStream_t &cs,
                                        const char *value);
    friend CompressStream_t &operator<<(CompressStream_t &cs,
                                        const char &value);
    friend CompressStream_t &operator<<(CompressStream_t &cs,
                                        CompressStream_t::Format_t value);

private:
    /**
     * @short return id for Identifier.
     * @param identifier which identifier
     * @return id for Identifier.
     */
    std::string getIdFor(const std::string &id);

    std::ostringstream os;     //< stream buffer for javascript source.
    bool endl;                 //< write endl to buffer.
    bool obfuscate;            //< do obfuscate.
    bool comment;              //< write origin Identifier in comment.
    bool ask;                  //< ask user whether obfuscate identifier.
    IdentifierMap_t idmap;     //< map of obfuscated identifiers.
    std::string prefix;        //< dont obfuscate identfiers with prefix
    StringSet_t blacklist;     //< set of blacklisted identifiers.
    std::string blacklistDump; //< dump blacklist to this file.
    Base62Int_t lastId;        //< last obfuscate id.
};

#endif /* COMPRESS_H */

