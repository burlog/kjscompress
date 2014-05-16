/*
 * FILE             $Id: util.h 2 2008-05-13 08:31:52Z burlog $
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

#ifndef UTIL_H
#define UTIL_H

#include <string>

/**
 * @short escape string.
 * @param str string to escape.
 * @return escaped string.
 */
std::string escape(const std::string &str);


#endif /* UTIL_H */

