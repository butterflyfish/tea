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

BUILDIR ?= build
OBJDIR := $(BUILDIR)/obj
DEPDIR := $(BUILDIR)/dep
BINDIR := $(BUILDIR)/bin
LIBDIR := $(BUILDIR)/lib

# JSON Compilation Database
CCDB := compile_commands.json

MKDIR := mkdir -p
RM := rm -rf

# arflags: flags for utility ar
arflags += -rc
RANLIB ?= ranlib

# color definition for printf
color_red := \e[1;31m
color_grn := \e[1;32m
color_yel := \e[1;33m
color_blu := \e[1;34m
color_mag := \e[1;35m
color_cyn := \e[1;36m
color_err := \e[0;37m
color_end := \e[0m

