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


# rules to compile c files under dir $2 for utility $1
# $(eval $(call rule-compile-dir-c,utility_name,dir)
define rule-compile-dir-c

# create necessary dir
$(shell $(MKDIR) $(OBJDIR)/$2)
$(shell $(MKDIR) $(DEPDIR)/$2)

$(eval t:=$(wildcard $2/*.c))
$1_OBJ+=$(t:%.c=$(OBJDIR)/%.o)

# grab dependency
-include $(DEPDIR)/$2/*.d

$(OBJDIR)/%.o: %.c
	$(quiet)$$(CC) -MM -MF $(DEPDIR)/$$*.d -MP -MT $$@ $$(cflags) $$(cppflags) $$<
	$(quiet)$(call colors,$(WHITE),Compile file $$<)
	$(quiet)$$(CC) $$(CLAGS) $$(cppflags) -c $$< -o $$@ > /dev/null

endef

# rules to compile c files for utility $1
# $(eval $(call rule-compile-c,utility_name,list)
define rule-compile-c

# create necessary dir
$(if $(findstring ./,$(dir $2)),, \
        $(shell $(MKDIR) $(OBJDIR)/$(dir $2)) $(shell $(MKDIR) $(DEPDIR)/$(dir $2)) )

# grab dependency
-include $(DEPDIR)/$(basename $2).d

$(OBJDIR)/$(basename $2).o: $2
	$(quiet)$$(CC) -MM -MF $(DEPDIR)/$(basename $2).d -MP -MT $$@ $$(cflags) $$(cppflags) $$<
	$(quiet)$(call colors,$(WHITE),Compile file $$<)
	$(quiet)$$(CC) $$(CLAGS) $$(cppflags) -c $$< -o $$@ > /dev/null

$1_OBJ+=$(OBJDIR)/$(basename $2).o

endef

# rules to produce final utility $1
# $(eval $(call rule-produce-bin))
define rule-produce-bin
$1: $($1_OBJ)
	$(quiet)$$(CC) $$^ $$(LDFLAGS) -o $$@
	$(quiet)$(MKDIR) $(BINDIR)
	$(quiet)mv $$@ $(BINDIR)
	$(quiet)echo $$@ is produced under $(BINDIR)
endef
