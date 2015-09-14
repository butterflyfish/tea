# makefile for Tea
# Copyright © 2015 Michael Zhu <boot2linux@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
# must display the following acknowledgement:
# This product includes software developed by the Tea.
# 4. Neither the name of the Tea nor the
# names of its contributors may be used to endorse or promote products
# derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY Michael Zhu <boot2linux@gmail.com> ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Michael Zhu <boot2linux@gmail.com> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# default value
quiet ?= @

PLAT_ARCH   := $(shell uname -m)
PLAT_KERNEL := $(shell uname -s)

# build directory
BUILDIR ?= build
OBJDIR := $(BUILDIR)/obj
DEPDIR := $(BUILDIR)/dep
BINDIR := $(BUILDIR)/bin
LIBDIR := $(BUILDIR)/lib

# Installation directories:
# By default, 'make install' will install all the files to
# '/usr/local/bin', '/usr/local/lib' etc.  You can specify
# an installation prefix by assigning PREFIX
PREFIX ?= /usr/local
bindir      := bin
sbindir     := sbin
libexecdir  := libexec
libdir      := lib
includedir  := include
sysconfdir  := etc
datadir     := share
infodir     := $(datadir)/info
localedir   := $(datadir)/lib
mandir      := $(datadir)/man
INSTALLDIRS := $(bindir) $(sbindir) $(libexecdir) $(libdir) $(includedir) \
               $(sysconfdir) $(datadir)

# existed directory list
INSTALLDIRS := $(patsubst $(BUILDIR)/%,%,$(wildcard $(BUILDIR)/$(INSTALLDIRS)))


# JSON Compilation Database
CCDB := $(BUILDIR)/compile_commands.json


# utility variables
RM     := rm -rf
MKDIR  := mkdir -p
RANLIB := ranlib


# Redefine flags to avoid conflict with user's local definitions
# CFLAGS      C compiler flags
# LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a
#             nonstandard directory <lib dir>
# CPPFLAGS    (Objective) C/C++ preprocessor flags, e.g. -I<include dir> if
#             you have headers in a nonstandard directory <include dir>
# CPP         C preprocessor
# arflags:    utility ar
cppflags := $(CPPFLAGS)
cflags   := $(CFLAGS)
ldflags  := $(LDFLAGS)
arflags  := rc


# color definition used by printf etc
color_red := \e[1;31m
color_grn := \e[1;32m
color_yel := \e[1;33m
color_blu := \e[1;34m
color_mag := \e[1;35m
color_cyn := \e[1;36m
color_err := \e[0;37m
color_end := \e[0m


# target to obtain value of variables
# example usage: make v-cflags v-cppflags
v-%:
	@printf "\n$(color_grn)variable: $(color_red)$*$(color_end)\n"
	@printf "value  = $(value  $*)\n"
	@printf "origin = $(origin $*)\n"
	@printf "flavor = $(flavor $*)\n"
