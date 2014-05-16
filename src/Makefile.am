#
# FILE             $Id: Makefile.am 9 2008-05-30 14:56:21Z burlog $
#
# PROJECT          KHTML JavaScript compress utility
#
# DESCRIPTION      Project makefile template.
#
# AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
#
#  LICENSE         see COPYING
# 
# Copyright (C) Seznam.cz a.s. 2007
# All Rights Reserved
#
# HISTORY
#       2007-03-09 (bukovsky)
#                  First draft.
#

CXXFLAGS = -Wall -g -Wno-deprecated

bin_PROGRAMS = kjscompress csscompress

EXTRA_DIST = blacklist.h compress.h decompress.h util.h

kjscompress_SOURCES = util.cc compress.cc decompress.cc main.cc

kjscompress_LDADD = -Lkjs -lkjs

csscompress_SOURCES = css.cc

csscompress_LDADD =
