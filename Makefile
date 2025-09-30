LIBNAME = foohash
MAJOR_VERSION = 1
MINOR_VERSION = 0

HDIR = include/
SRCS = $(filter-out test%, $(wildcard *.c *.y $(HDIR)foo/*.h))
#MANS = zma.3 rbtree.3

#CFLAGS = -march=native -O2 -pipe -D_GNU_SOURCE -ferror-limit=3
CFLAGS = -march=native -g -O0 -pipe -D_GNU_SOURCE -ferror-limit=3 -Wall
#CPPFLAGS = -DNDEBUG -I. -I$(HDIR) -I/usr/include -I/usr/local/include
CPPFLAGS = -I. -I$(HDIR) -I/usr/include -I/usr/local/include
LDFLAGS = -L/usr/lib -L/usr/local/lib
LDLIBS = -lm -lpthread

ASAN = 1
UBSAN = 1

ifneq ($(ASAN),)
CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address
endif
ifneq ($(UBSAN),)
CFLAGS += -fsanitize=undefined
LDFLAGS += -fsanitize=undefined
endif

ifneq ($(WITH_GPROF),)
CFLAGS += -pg
LDFLAGS += -pg
endif

OBJDIR = obj
LIB_STATIC = lib$(LIBNAME).a
LIB_SHARED = lib$(LIBNAME).so.$(MAJOR_VERSION).$(MINOR_VERSION)
YSRCS = $(filter %.y,$(SRCS))
CSRCS = $(filter %.c,$(SRCS))
HSRCS = $(filter %.h,$(SRCS))

DESTDIR = /usr/local/
BINDIR = $(DESTDIR)bin/
LIBDIR = $(DESTDIR)lib/
INCLUDEDIR = $(DESTDIR)include/
MANDIR = $(DESTDIR)man/

DIRS += $(abspath $(OBJDIR))
DIRS += $(abspath $(DESTDIR))
DIRS += $(abspath $(BINDIR))
DIRS += $(abspath $(LIBDIR))
DIRS += $(abspath $(INCLUDEDIR))
DIRS += $(abspath $(MANDIR))

CC = cc
LD = $(CC)
YACC = bison
YFLAGS = 
INSTALL = install

.DEFAULT_GOAL := all

define c2o_recipe =
$$(CC) -c -MT $$@ -MMD -MP -MF $$@.depend $$(CFLAGS) $$(CPPFLAGS) $$(TARGET_ARCH) $$< -o $$@ && touch $$@
endef
define c2po_recipe =
$$(CC) -c -MT $$@ -MMD -MP -MF $$@.depend $$(CFLAGS) $$(CPPFLAGS) -fpic -DPIC $$(TARGET_ARCH) $$< -o $$@ && touch $$@
endef

define mkytoc =
$$(OBJDIR)/$(1:%.y=%.tab.c): $(1) | $$(abspath $$(OBJDIR)/$(dir $(1))); $$(YACC) $$(YFLAGS) --header=$$*.h --output $$@ $$<
$$(OBJDIR)/$(1:%.y=%.tab.h):;
GENERATED += $$(OBJDIR)/$(1:%.y=%.tab.c) $$(OBJDIR)/$(1:%.y=%.tab.h)
endef
$(foreach y,$(YSRCS),$(eval $(call mkytoc,$y)))

define mkyrules_static =
$$(OBJDIR)/$(1:%.y=%.tab.o): $$(OBJDIR)/$(1:%.y=%.tab.c) $$(OBJDIR)/$(1:%.y=%.tab.h) $$(OBJDIR)/$(1:%.y=%.tab.o.depend); $(call c2o_recipe)
OBJS_STATIC += $$(OBJDIR)/$(1:%.y=%.tab.o)
DEPS += $$(OBJDIR)/$(1:%.y=%.tab.o.depend)
DIRS += $$(abspath $$(OBJDIR)/$(dir $(1)))
endef
$(foreach y,$(YSRCS),$(eval $(call mkyrules_static,$y)))

define mkyrules_shared =
$$(OBJDIR)/$(1:%.y=%.tab.po): $$(OBJDIR)/$(1:%.y=%.tab.c) $$(OBJDIR)/$(1:%.y=%.tab.h) $$(OBJDIR)/$(1:%.y=%.tab.po.depend); $(call c2po_recipe)
OBJS_SHARED += $$(OBJDIR)/$(1:%.y=%.tab.po)
DEPS += $$(OBJDIR)/$(1:%.y=%.tab.po.depend)
DIRS += $$(abspath $$(OBJDIR)/$(dir $(1)))
endef
$(foreach y,$(YSRCS),$(eval $(call mkyrules_shared,$y)))

define mkcrules_static =
$$(OBJDIR)/$(1:%.c=%.o): $(1) $$(OBJDIR)/$(1:%.c=%.o.depend) | $$(abspath $$(OBJDIR)/$(dir $(1))); $(call c2o_recipe)
OBJS_STATIC += $$(OBJDIR)/$(1:%.c=%.o)
DEPS += $$(OBJDIR)/$(1:%.c=%.o.depend)
DIRS += $$(abspath $$(OBJDIR)/$(dir $(1)))
endef
$(foreach c,$(CSRCS),$(eval $(call mkcrules_static,$c)))

define mkcrules_shared =
$$(OBJDIR)/$(1:%.c=%.po): $(1) $$(OBJDIR)/$(1:%.c=%.po.depend) | $$(abspath $$(OBJDIR)/$(dir $(1))); $(call c2po_recipe)
OBJS_SHARED += $$(OBJDIR)/$(1:%.c=%.po)
DEPS += $$(OBJDIR)/$(1:%.c=%.po.depend)
DIRS += $$(abspath $$(OBJDIR)/$(dir $(1)))
endef
$(foreach c,$(CSRCS),$(eval $(call mkcrules_shared,$c)))

define mkhrules =
install-include: | $$(abspath $$(INCLUDEDIR)$(dir $(patsubst $(2)%,%,$(1))))
DIRS += $$(abspath $$(INCLUDEDIR)$(dir $(patsubst $(2)%,%,$(1))))
endef
$(foreach h,$(HSRCS),$(eval $(call mkhrules,$h,$(HDIR))))

define mkmanrules = 
$$(OBJDIR)/$(1).gz: $(1) | $$(abspath $$(OBJDIR)/$(dir $(1))); gzip -cn $$< > $$@
DIRS += $$(abspath $$(OBJDIR)/$(dir $(1)))
install-man: | $$(abspath $$(MANDIR)/man$(patsubst .%,%,$(suffix $(notdir $(1)))))
DIRS += $$(abspath $$(MANDIR)/man$(patsubst .%,%,$(suffix $(notdir $(1)))))
GENERATED += $$(OBJDIR)/$(1).gz
endef
$(foreach m,$(MANS),$(eval $(call mkmanrules,$m)))


.PHONY: foo
foo:
	@echo "SRCS  = $(SRCS)"
	@echo "CSRCS = $(CSRCS)"
	@echo "YSRCS = $(YSRCS)"
	@echo "OBJS_STATIC  = $(OBJS_STATIC)"
	@echo "OBJS_SHARED  = $(OBJS_SHARED)"
	@echo "DEPS  = $(DEPS)"
	@echo "DIRS  = $(DIRS)"

$(OBJDIR)/$(LIB_STATIC): $(OBJS_STATIC); $(AR) -cr $@ $^ && ranlib -U $@
$(OBJDIR)/$(LIB_SHARED): $(OBJS_SHARED); $(LD) -shared -fpic -fno-lto $(LDFLAGS) $^ $(LDLIBS) -o $@

$(sort $(DIRS)):; mkdir -p $@
$(DEPS):
ifneq ($(MAKECMDGOALS),clean)
include $(DEPS)
endif

.PHONY: all
all: $(OBJDIR)/$(LIB_STATIC) $(OBJDIR)/$(LIB_SHARED) $(MANS:%=$(OBJDIR)/%.gz)

.PHONY: install
install: install-static install-shared install-include install-man

.PHONY: install-static
install-static: $(OBJDIR)/$(LIB_STATIC) | $(abspath $(LIBDIR))
	$(INSTALL) -m 755 $^ $(LIBDIR)

.PHONY: install-shared
install-shared: $(OBJDIR)/$(LIB_SHARED) | $(abspath $(LIBDIR))
	$(INSTALL) -m 755 $^ $(LIBDIR)
	cd $(LIBDIR) && ln -sf $(LIB_SHARED) lib$(LIBNAME).so
	cd $(LIBDIR) && ln -sf $(LIB_SHARED) lib$(LIBNAME).so.$(MAJOR_VERSION)

.PHONY: install-include
install-include: $(HSRCS) | $(abspath $(INCLUDEDIR))
	for h in $^; do $(INSTALL) -m 444 $$h $(INCLUDEDIR)$${h#$(HDIR)}; done

.PHONY: install-man
install-man: $(MANS:%=$(OBJDIR)/%.gz) | $(abspath $(MANDIR))
	for m in $(MANS); do $(INSTALL) -m 444 $$m $(MANDIR)man$${m##*.}/; done

.PHONY: clean
clean:
	@-for f in 	$(GENERATED) \
			$(DEPS) \
			$(OBJS_STATIC) \
			$(OBJDIR)/$(LIB_STATIC) \
			$(OBJS_SHARED) \
			$(OBJDIR)/$(LIB_SHARED) \
			$(OBJS) $(TESTS); \
	do \
		unlink $$f 2>/dev/null && echo "unlink $$f"; \
	done

TESTS_SRCS = $(wildcard test*.c test/*.c)

define mk_test_rules =
$$(OBJDIR)/$1.o: $1.c | $$(abspath $$(OBJDIR)/$(dir $1)); $(call c2o_recipe)
$$(OBJDIR)/$1: .EXTRA_PREREQS = $$(OBJDIR)/$$(LIB_STATIC) $$(OBJDIR)/$$(LIB_SHARED)
$$(OBJDIR)/$1: $$(OBJDIR)/$1.o ; $$(CC) -Wl,-rpath=$$(realpath $$(OBJDIR)) -L$$(OBJDIR) $$(LDFLAGS) $$^ -l$$(LIBNAME) -o $$@
TESTS += $$(OBJDIR)/$1
OBJS += $$(OBJDIR)/$1.o
DEPS += $$(OBJDIR)/$1.o.depend
DIRS += $$(abspath $$(OBJDIR)/$(dir $1))
endef
$(foreach T,$(TESTS_SRCS:%.c=%),$(eval $(call mk_test_rules,$T)))

.PHONY: tests
tests: $(TESTS)


