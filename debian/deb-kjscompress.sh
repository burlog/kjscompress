#!/bin/bash
#
# FILE             $Id: deb-kjscompress.sh 19 2011-02-23 14:32:54Z volca $
#
# PROJECT          KHTML JavaScript compress utility
#
# DESCRIPTION      Packager.
#
# AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
#
#  LICENSE         see COPYING
# 
# Copyright (C) Seznam.cz a.s. 2006
# All Rights Reserved
#
# HISTORY
#       2007-03-29 (bukovsky)
#                  First draft.
#

set +x

DEB_PCK_NAME='kjscompress'

MAINTAINER='Michal Bukovsky <michal.bukovsky@firma.seznam.cz>'
DEBIAN_BASE=tmp/$DEB_PCK_NAME
PROJECT_DIR=/usr/
WORK_DIR=$DEBIAN_BASE$PROJECT_DIR

# make directories
rm -r $DEBIAN_BASE 2>/dev/null
mkdir -p ${DEBIAN_BASE}/DEBIAN
chmod 0755 $DEBIAN_BASE/DEBIAN

# compile
(
    rm -rf build
    mkdir build
    cd build
    ../../configure --enable-optimization --prefix=${PROJECT_DIR}
    make DESTDIR=`pwd`/../${DEBIAN_BASE} install
    cd ..

) || (echo "*** Failed to build kjscompress!"; exit 1)

# make package
. make-package.sh && rm -rf build
