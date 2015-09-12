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

-include Config.mk config.mk

.PHONY: $(LIBAR) $(BIN)

all: $(LIBAR) $(BIN)

# find Where Makefile is
where=$(dir $(shell readlink Makefile))

# Redefine flags to avoid conflict with user's local definitions
cppflags := $(CPPFLAGS)
cflags   := $(CFLAGS)
arflags  := $(ARFLAGS)
ldflags  := $(LDFLAGS)

# load library
include $(where)/vars.mk
include $(where)/function.mk
include $(where)/c.mk

# include additional platform specific stuff
-include $(where)/$(PLAT_KERNEL).mk
-include $(where)/$(PLAT_KERNEL)_$(PLAT_ARCH).mk

# TODO:
# support multiple extension, e.g. .c .s
# generate rules based on extension
SRCEXT:=.c

# generate rules for each $(BIN)
ifneq ($(BIN),)

# find source list and then generate rule for each $(BIN)
# append archive library as depency
# |: it indicates $<,$^ etc don't contain dependency after !
$(foreach b, $(BIN), \
    $(if $($b_SRC), $(eval $(call find-src,$b,$(SRCEXT)))    \
                    $(eval $(foreach f, $($b_SRC), \
                                $(call rule-compile-c,$b,$f)))) \
    $(if $($b_LIBAR), $(eval $b: |$($b_LIBAR))) \
)

# must foreach again to ensure its dependency object list is ready
$(eval $(foreach b, $(BIN), $(call rule-produce-bin,$b)))

endif

# generate rules for each $(LIBAR)
ifneq ($(LIBAR),)

# find source list and then generate rule for each archive
$(foreach a, $(LIBAR), \
    $(if $($a_SRC), $(eval $(call find-src,$a,$(SRCEXT))) \
                    $(eval $(foreach f, $($a_SRC), \
                                $(call rule-compile-c,$a,$f)))) \
)

# must foreach again to ensure its dependency object list is ready
$(eval $(foreach a, $(LIBAR), $(call rule-libar,$a)))

endif

# generate JSON Compilation Database after target all
ifneq ($(export_compile_command),)
.PHONY: $(CCDB)
all: |$(CCDB)

# generate JSON Compilation Database
$(CCDB):
	$(quiet)$(call generate-cc-db)
endif

clean:
	$(quiet)$(RM) $(BUILDIR)
	$(quiet)$(RM) $(CCDB)
