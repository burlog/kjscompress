#!/usr/bin/python
#
# FILE             $Id: ADD_COMPRESS_METHOD_TO_NODES.py 2 2008-05-13 08:31:52Z burlog $
#
# PROJECT          KHTML JavaScript compress utility
#
# DESCRIPTION      Script which add compress method to nodes.h
#
# AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
#
# Copyright (C) Seznam.cz a.s. 2006
# All Rights Reserved
#
# HISTORY
#       2007-02-26 (bukovsky)
#                       Frist draft
#/

import sys

print "class CompressStream_t;"
print "class DeCompressStream_t;"
for line in sys.stdin.readlines():
    if line.strip() == "virtual void streamTo(SourceStream &s) const = 0;":
        print "virtual void streamTo(SourceStream &s) const = 0;"
        print "virtual void streamTo(CompressStream_t &s) const = 0;"
        print "virtual void streamTo(DeCompressStream_t &s) const = 0;"
    elif line.strip() == "virtual void streamTo(SourceStream &s) const;":
        print "virtual void streamTo(SourceStream &s) const;"
        print "virtual void streamTo(CompressStream_t &s) const;"
        print "virtual void streamTo(DeCompressStream_t &s) const;"
    else:
        print line.rstrip()
    #endif
#endfor

