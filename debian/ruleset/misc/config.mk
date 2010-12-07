######################### -*- Mode: Makefile-Gmake -*- ########################
## config.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 17:30:53 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Tue Jan  3 19:37:01 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 1
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : Various internal variable set based on defaults and the
##                    user configuration files, or from environment vars.
## 
## arch-tag: 5fac76ea-f1e8-49fe-bd82-12ae6be8d701
## 
## 
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
##
###############################################################################


# The Debian revision
# If there is a changelog file, it overrides. The only exception is
# when there is no stamp-config, and there is no debian/official,
# *AND* there is a DEBIAN_REVISION, in which case the DEBIAN_REVISION
# over rides (since we are going to replace the changelog file soon
# anyway.  Else, use the commandline or env var setting. Or else
# default to 10.00.Custom, unless the human has requested that the
# revision is mandatory, in which case we raise an error

ifeq ($(strip $(HAS_CHANGELOG)),YES)
  debian := $(shell if test -f debian/changelog; then \
                     perl -nle 'print /\((\S+)\)/; exit 0' debian/changelog;\
                  fi; )
else
  ifneq ($(strip $(DEBIAN_REVISION)),)
    debian := $(DEBIAN_REVISION)
  else
    ifeq ($(strip $(debian)),)
      ifneq ($(strip $(debian_revision_mandatory)),)
        $(error A Debian revision is mandatory, but none was provided)
      else
        debian = $(version)-10.00.Custom
      endif
    endif
  endif
endif



# See if the version numbers are valid
$(eval $(which_debdir))
HAVE_VALID_PACKAGE_VERSION := $(shell                           \
      if test -x $(DEBDIR)/scripts/kpkg-vercheck; then          \
        if test -n "$(debian)"; then                            \
	   $(DEBDIR)/scripts/kpkg-vercheck $(debian) ;          \
        else                                                    \
           echo "YES";                                          \
        fi                                                      \
      else                                                      \
        echo "Could not find $(DEBDIR)/scripts/kpkg-vercheck" ; \
      fi )

TAR_COMPRESSION := $(shell                                             \
      if tar --help | grep -- \-\-bzip2 >/dev/null; then echo --bzip2; \
      else                                               echo --gzip;  \
      fi )
TAR_SUFFIX := $(shell                                                  \
      if tar --help | grep -- \-\-bzip2 >/dev/null; then echo bz2;     \
      else                                               echo gz;      \
      fi )

STOP_FOR_BIN86 = NO
CONTROL=$(LIBLOC)/Control

ifeq ($(strip $(architecture)),i386)
NEED_BIN86 := $(shell if dpkg --compare-versions                   \
                  $(VERSION).$(PATCHLEVEL) lt 2.4 >/dev/null 2>&1; \
                  then echo YES; fi)
ifeq ($(strip $(NEED_BIN86)),YES)
CONTROL=$(LIBLOC)/Control.bin86
HAVE_BIN86 := $(shell if test -x /usr/bin/as86; then echo YES; else echo NO; fi )
ifeq ($(strip $(HAVE_BIN86)),NO)
STOP_FOR_BIN86 = YES
endif
endif
endif


# Over ride the config file from the environment/command line
ifneq ($(strip $(KPKG_MAINTAINER)),)
maintainer=$(KPKG_MAINTAINER)
endif

ifneq ($(strip $(KPKG_EMAIL)),)
email=$(KPKG_EMAIL)
endif

# This should be a  name to feed the modules build for pgp signature,
# since we the maintainer would be different there.
ifneq ($(strip $(PGP_SIGNATURE)),)
pgp=$(PGP_SIGNATURE)
endif

ifneq ($(strip $(EXTRA_DOCS)),)
extra_docs = $(EXTRA_DOCS)
endif

ifneq ($(strip $(extra_docs)),)
HAVE_EXTRA_DOCS:=$(shell if [ -e $(extra_docs) ]; then echo YES; fi)
endif

ifneq ($(strip $(DEBIAN_REVISION_MANDATORY)),)
debian_revision_mandatory:=$(DEBIAN_REVISION_MANDATORY)
endif


ifneq ($(strip $(install_vmlinux)),)
int_install_vmlinux:=$(install_vmlinux)
endif

ifneq ($(strip $(KPKG_FOLLOW_SYMLINKS_IN_SRC)),)
int_follow_symlinks_in_src=YES
else
ifneq ($(strip $(kpkg_follow_symlinks_in_src)),)
int_follow_symlinks_in_src=YES
endif
endif





ifneq ($(strip $(DEBIAN_REVISION)),)
  HAS_CHANGELOG := $(shell \
    if test -f debian/changelog && ( test -f stamp-debian || test -f debian/official );\
    then echo YES;\
    else echo NO; fi; )
else
  HAS_CHANGELOG := $(shell if test -f debian/changelog; then echo YES;\
                          else echo NO; fi; )
endif

# Hmm. The version that we have computed *MUST* match the one that is in the
# changelog.
ifeq ($(strip $(HAS_CHANGELOG)),YES)
  saved_version := $(shell if test -f debian/changelog; then \
                     perl -nle 'print /^$(INT_STEM)-source-(\S+)/; exit 0' \
                          debian/changelog;\
                  fi; )
# Warn people about version mismatches, unless they are running an
# "official" version, in which case they can shoot themselves in the
# foot if they so desire
  ifneq ($(strip $(saved_version)),)
    ifneq ($(strip $(saved_version)),$(strip $(version)))
      HAVE_VERSION_MISMATCH:=$(shell if test ! -f debian/official;then echo YES; fi; )
    endif
  endif
endif


ifneq ($(strip $(DELETE_BUILD_LINK)),)
delete_build_link := YES
else
ifeq ($(strip $(delete_build_link)),)
delete_build_link := $(shell if test -f debian/official; then echo YES;\
                          else echo NO; fi; )
endif
endif

ifneq ($(strip $(IMAGE_IN_BOOT)),)
link_in_boot := $(IMAGE_IN_BOOT)
endif

ifneq ($(strip $(LINK_IN_BOOT)),)
link_in_boot := $(LINK_IN_BOOT)
endif

ifneq ($(strip $(NO_SYMLINK)),)
no_symlink := $(NO_SYMLINK)
endif

ifneq ($(strip $(REVERSE_SYMLINK)),)
reverse_symlink := $(REVERSE_SYMLINK)
endif

ifneq ($(strip $(IMAGE_TYPE)),)
kimage = $(IMAGE_TYPE)
endif

ifneq ($(strip $(PATCH_THE_KERNEL)),)
patch_the_kernel = $(PATCH_THE_KERNEL)
endif

ifneq ($(strip $(KPKG_SELECTED_PATCHES)),)
ifeq ($(strip $(patch_the_kernel)),NO)
patch_the_kernel = NO
else
ifeq ($(strip $(patch_the_kernel)),no)
patch_the_kernel = NO
else
patch_the_kernel = YES
endif
endif
endif


ifeq ($(strip $(patch_the_kernel)),yes)
patch_the_kernel = YES
endif
ifeq ($(strip $(patch_the_kernel)),Yes)
patch_the_kernel = YES
endif
ifeq ($(strip $(patch_the_kernel)),YEs)
patch_the_kernel = YES
endif
ifeq ($(strip $(patch_the_kernel)),yEs)
patch_the_kernel = YES
endif
ifeq ($(strip $(patch_the_kernel)),yES)
patch_the_kernel = YES
endif
ifeq ($(strip $(patch_the_kernel)),yeS)
patch_the_kernel = YES
endif



ifneq ($(strip $(CONFIG_TARGET)),)
config_target := $(CONFIG_TARGET)
have_new_config_target := YES
endif

# If config_target doesn't end in 'config' then reset it to 'oldconfig'.
ifneq ($(patsubst %config,config,$(strip $(config_target))),config)
config_target := oldconfig
have_new_config_target :=
endif

ifneq ($(strip $(USE_SAVED_CONFIG)),)
use_saved_config = $(USE_SAVED_CONFIG)
endif

#ifeq ($(origin var),command line)
#$(warn You are setting an internal var from the cmdline. Use at your own risk)
#endif
#you can automated it a bit more with $(foreach) and $(if)


###
### In the following, we define these variables
### ROOT_CMD      -- set in the environment, plaing old sudo or fakeroot
### root_cmd      -- The same
### int_root_cmd  -- the argument passed to dpkg-buildpackage
###                  -r$(ROOT_CMD)
ifneq ($(strip $(ROOT_CMD)),)
 # ROOT_CMD is not supposed to have -r or -us and -uc
 int_dummy_root := $(ROOT_CMD)
 # remove -us and -uc
 ifneq ($(strip $(findstring -us, $(int_dummy_root))),)
   int_dummy_root := $(subst -us,, $(strip $(int_dummy_root)))
   int_us := -us
 endif
 ifneq ($(strip $(findstring -uc, $(int_dummy_root))),)
   int_dummy_root := $(subst -uc,, $(strip $(int_dummy_root)))
   int_uc := -uc
 endif
 ifneq ($(strip $(findstring -r, $(int_dummy_root))),)
   int_dummy_root := $(subst -r,, $(strip $(int_dummy_root)))
 endif
 # sanitize
 ROOT_CMD     :=   $(strip $(int_dummy_root))
 int_root_cmd := -r$(strip $(int_dummy_root))
else
  # well, ROOT_CMD is not set yet
  ifneq ($(strip $(root_cmd)),)
    # Try and set ROOT_CMD from root_cmd
    int_dummy_root := $(root_cmd)
    # remove -us and -uc
    ifneq ($(strip $(findstring -us, $(int_dummy_root))),)
      int_dummy_root := $(subst -us,, $(strip $(int_dummy_root)))
      int_us := -us
    endif
    ifneq ($(strip $(findstring -uc, $(int_dummy_root))),)
      int_dummy_root := $(subst -uc,, $(strip $(int_dummy_root)))
      int_uc := -uc
    endif
    # now that -us and -uc are gone, remove -r
    ifneq ($(strip $(findstring -r, $(int_dummy_root))),)
      int_dummy_root := $(subst -r,, $(strip $(int_dummy_root)))
    endif
    # Finally, sanitized
    ROOT_CMD     :=   $(strip $(int_dummy_root))
    int_root_cmd := -r$(strip $(int_dummy_root))
  endif
endif

# make sure that root_cmd and ROOT_CMD are the same
ifneq ($(strip $(ROOT_CMD)),)
  root_cmd := $(ROOT_CMD)
endif

ifneq ($(strip $(UNSIGN_SOURCE)),)
  int_us := -us
endif

ifneq ($(strip $(UNSIGN_CHANGELOG)),)
  int_uc := -uc
endif

int_am_root  := $(shell [ $$(id -u) -eq 0 ] && echo "YES" )


ifneq ($(strip $(CLEAN_SOURCE)),)
do_clean = $(CLEAN_SOURCE)
endif

ifneq ($(strip $(CONCURRENCY_LEVEL)),)
do_parallel = -j$(CONCURRENCY_LEVEL)

# Well, I wish there was something better than guessing by version number
CAN_DO_DEP_FAST=$(shell if   [ $(VERSION) -lt 2 ];    then echo '';  \
                        elif [ $(VERSION) -gt 2 ];    then echo YES; \
                        elif [ $(PATCHLEVEL) -lt 4 ]; then echo '';  \
                        else                             echo YES; \
                        fi)
ifneq ($(strip $(CAN_DO_DEP_FAST)),)
fast_dep= -j$(CONCURRENCY_LEVEL)
endif

endif

ifneq ($(strip $(SOURCE_CLEAN_HOOK)),)
source_clean_hook=$(SOURCE_CLEAN_HOOK)
endif
ifneq ($(strip $(HEADER_CLEAN_HOOK)),)
header_clean_hook=$(HEADER_CLEAN_HOOK)
endif
ifneq ($(strip $(DOC_CLEAN_HOOK)),)
doc_clean_hook=$(DOC_CLEAN_HOOK)
endif
ifneq ($(strip $(IMAGE_CLEAN_HOOK)),)
image_clean_hook=$(IMAGE_CLEAN_HOOK)
endif

#Local variables:
#mode: makefile
#End:
