# Rip some version information from our changelog
release	:= $(shell sed -n '1s/^.*(\(.*\)-.*).*$$/\1/p' debian/changelog)
pkgversion := $(shell sed -n '1s/^linux-source-\([^ ]*\) .*/\1/p' debian/changelog)
revisions := $(shell sed -n 's/^linux-source-$(pkgversion)\ .*($(release)-\(.*\)).*$$/\1/p' debian/changelog | tac)
revision ?= $(word $(words $(revisions)),$(revisions))
prev_revisions := $(filter-out $(revision),0.0 $(revisions))
prev_revision := $(word $(words $(prev_revisions)),$(prev_revisions))

# This is an internally used mechanism for the daily kernel builds. It
# creates packages who's ABI is suffixed with a minimal representation of
# the current git HEAD sha. If .git/HEAD is not present, then it uses the
# uuidgen program,
#
# AUTOBUILD can also be used by anyone wanting to build a custom kernel
# image, or rebuild the entire set of Ubuntu packages using custom patches
# or configs.
export AUTOBUILD
ifeq ($(AUTOBUILD),)
abi_suffix	=
else
skipabi		= true
skipmodule	= true
gitver=$(shell if test -f .git/HEAD; then cat .git/HEAD; else uuidgen; fi)
gitverpre=$(shell echo $(gitver) | cut -b -3)
gitverpost=$(shell echo $(gitver) | cut -b 38-40)
abi_suffix = -$(gitverpre)$(gitverpost)
endif

ifneq ($(NOKERNLOG),)
ubuntu_log_opts += --no-kern-log
endif
ifneq ($(PRINTSHAS),)
ubuntu_log_opts += --print-shas
endif

abinum		:= $(shell echo $(revision) | sed -e 's/\..*//')$(abisuffix)
prev_abinum	:= $(shell echo $(prev_revision) | sed -e 's/\..*//')$(abisuffix)
debnum		:= -$(abinum)

# We force the sublevel to be exactly what we want. The actual source may
# be an in development git tree. We want to force it here instead of
# committing changes to the top level Makefile
SUBLEVEL	:= $(shell echo $(release) | awk -F. '{print $$3}')

export abinum debnum version

arch		:= $(shell dpkg-architecture -qDEB_HOST_ARCH)
abidir		:= $(CURDIR)/debian/abi/$(release)-$(revision)/$(arch)
prev_abidir	:= $(CURDIR)/debian/abi/$(release)-$(prev_revision)/$(arch)
confdir		:= $(CURDIR)/debian/config/$(arch)
builddir	:= $(CURDIR)/debian/build
stampdir	:= $(CURDIR)/debian/stamps

# Support parallel=<n> in DEB_BUILD_OPTIONS (see #209008)
COMMA=,
ifneq (,$(filter parallel=%,$(subst $(COMMA), ,$(DEB_BUILD_OPTIONS))))
  CONCURRENCY_LEVEL := $(subst parallel=,,$(filter parallel=%,$(subst $(COMMA), ,$(DEB_BUILD_OPTIONS))))
endif

ifeq ($(CONCURRENCY_LEVEL),)
  # Check the environment
  CONCURRENCY_LEVEL := $(shell echo $$CONCURRENCY_LEVEL)
  # No? Check if this is on a buildd
  ifeq ($(CONCURRENCY_LEVEL),)
    ifneq ($(wildcard /CurrentlyBuilding),)
      CONCURRENCY_LEVEL := $(shell expr `getconf _NPROCESSORS_ONLN` \* 2)
    endif
  endif
  # Oh hell, give 'em one
  ifeq ($(CONCURRENCY_LEVEL),)
    CONCURRENCY_LEVEL := 1
  endif
endif

conc_level		= -j$(CONCURRENCY_LEVEL)

# taget_flavour is filled in for each step
kmake = make ARCH=$(build_arch) EXTRAVERSION=$(debnum)-$(target_flavour) \
	SUBLEVEL=$(SUBLEVEL)

#all_custom_flavours = xen rt ume cell lpiacompat lpia

# Checks if a var is overriden by the custom rules. Called with var and
# flavour as arguments.
custom_override = \
 $(shell if [ -n "$($(1)_$(2))" ]; then echo "$($(1)_$(2))"; else echo "$($(1))"; fi)
