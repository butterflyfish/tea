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


# find src files with extension ext under name_SRC
# name_SRC can contain file list and dir list
# $(eval $(call find-src,name,ext)
define find-src
$(eval dir:=$(filter-out %$2,$($1_SRC)))
$1_SRC:=$(filter %$2,$($1_SRC))
$1_SRC+=$(foreach d,$(dir),$(wildcard $d/*$2))
endef

# export compile command
# please refer to http://clang.llvm.org/docs/JSONCompilationDatabase.html
# $(call export_compile_command,how_to_cc,src)
define export_compile_command
echo "    {"  >> $(OBJDIR)/$(basename $2).json;  \
echo "        \"command\": \"$1\"," >> $(OBJDIR)/$(basename $2).json;  \
echo "        \"directory\": \"$(PWD)\"," >> $(OBJDIR)/$(basename $2).json; \
echo "        \"file\": \"$(PWD)/$2\"" >> $(OBJDIR)/$(basename $2).json;  \
echo "    }," >> $(OBJDIR)/$(basename $2).json;
endef

# generate JSON Compilation Database
# please refer to http://clang.llvm.org/docs/JSONCompilationDatabase.html
define generate-cc-db
echo "[" > $(CCDB); \
find $(OBJDIR) -type f -iname "*.json"|xargs cat >> $(CCDB) ;\
echo "]" >> $(CCDB)
endef
