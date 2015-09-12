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


# rules to compile c files for owner $1
# $(eval $(call rule-compile-c,owner,list)
define rule-compile-c

# create necessary dir
$(if $(findstring ./,$(dir $2)),, \
        $(shell $(MKDIR) $(OBJDIR)/$(dir $2)) $(shell $(MKDIR) $(DEPDIR)/$(dir $2)) )

# grab dependency
-include $(DEPDIR)/$(basename $2).d

$(OBJDIR)/$(basename $2).o: $2
	$(quiet)printf "Compile file $(color_grn)$$<$(color_end)\n"
	$(quiet)$$(CC) -MM -MF $(DEPDIR)/$(basename $2).d -MP -MT $$@ $$(cflags) $$(cppflags) $$<
	$(quiet)$$(CC) $$(cflags) $$(cppflags) -c $$< -o $$@ > /dev/null
	$(quiet)$(call export_compile_command,$$(CC) $$(cflags) $$(cppflags) -c $$< -o $$@,$2)

$1_OBJ+=$(OBJDIR)/$(basename $2).o

endef

# rules to produce archive
# object rules should use function rule-compile-c
# $(eval $(call rule rule-libar,ar_name))
define rule-libar
$1: $($1_OBJ)
	$(quiet)$(MKDIR) $(LIBDIR)
	$(quiet)$$(AR) $$(arflags) $(LIBDIR)/lib$$@.a $$< >/dev/null
	$(quiet)$$(RANLIB) $(LIBDIR)/lib$$@.a  >/dev/null
	$(quiet)printf "Create archive $(color_grn)$(LIBDIR)/lib$$@.a$(color_end)\n"
endef

# rules to produce final utility $1
# $(eval $(call rule-produce-bin))
define rule-produce-bin
$1: $($1_OBJ)
	$(quiet)$$(CC) $$^  $(foreach a,$($1_LIBAR),$(LIBDIR)/lib$a.a) $$(ldflags) -o $$@
	$(quiet)$(MKDIR) $(BINDIR)
	$(quiet)mv $$@ $(BINDIR)
	$(quiet)printf "$(color_blu)$$@$(color_end) is produced under $(BINDIR)\n"
endef
