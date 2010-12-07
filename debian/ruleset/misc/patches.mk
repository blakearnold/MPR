######################### -*- Mode: Makefile-Gmake -*- ########################
## patches.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 18:09:46 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Mon Oct 31 18:09:46 2005
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 0
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : deals with setting up variables, looking at
##                    directories, and creating a list of valid third party
##                    patches available for the kernel being built.
## 
## arch-tag: 0cdf8272-2183-49dd-abb7-e64ef8f0b482
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

ifeq ($(strip $(VERSIONED_PATCH_DIR)),)
VERSIONED_PATCH_DIR         = $(shell if [ -d \
/usr/src/kernel-patches/$(architecture)/$(VERSION).$(PATCHLEVEL).$(SUBLEVEL) \
			       ]; then echo \
/usr/src/kernel-patches/$(architecture)/$(VERSION).$(PATCHLEVEL).$(SUBLEVEL); \
			    fi)
endif

ifeq ($(strip $(VERSIONED_ALL_PATCH_DIR)),)
VERSIONED_ALL_PATCH_DIR         = $(shell if [ -d \
/usr/src/kernel-patches/all/$(VERSION).$(PATCHLEVEL).$(SUBLEVEL) \
			       ]; then echo \
/usr/src/kernel-patches/all/$(VERSION).$(PATCHLEVEL).$(SUBLEVEL); \
			    fi)
endif

ifeq ($(strip $(PATCH_DIR)),)
PATCH_DIR  = $(shell if [ -d /usr/src/kernel-patches/$(architecture)/ ];\
                        then echo /usr/src/kernel-patches/$(architecture); \
	             fi)
endif

ifeq ($(strip $(ALL_PATCH_DIR)),)
ALL_PATCH_DIR  = $(shell if [ -d /usr/src/kernel-patches/all/ ]; \
                            then echo /usr/src/kernel-patches/all ;\
			 fi)
endif

VERSIONED_ALL_PATCH_APPLY   = $(VERSIONED_ALL_PATCH_DIR)/apply
VERSIONED_ALL_PATCH_UNPATCH = $(VERSIONED_ALL_PATCH_DIR)/unpatch

VERSIONED_DIR_PATCH_APPLY   = $(VERSIONED_PATCH_DIR)/apply
VERSIONED_DIR_PATCH_UNPATCH = $(VERSIONED_PATCH_DIR)/unpatch

ALL_PATCH_APPLY   = $(ALL_PATCH_DIR)/apply
ALL_PATCH_UNPATCH = $(ALL_PATCH_DIR)/unpatch

DIR_PATCH_APPLY   = $(PATCH_DIR)/apply
DIR_PATCH_UNPATCH = $(PATCH_DIR)/unpatch

ifeq ($(strip $(patch_the_kernel)),YES)

# Well then. Let us see if we want to select the patches we apply.
ifneq ($(strip $(KPKG_SELECTED_PATCHES)),)
canonical_patches=$(subst $(comma),$(space),$(KPKG_SELECTED_PATCHES))

ifneq ($(strip $(canonical_patches)),)
# test to see if the patches exist
temp_valid_patches = $(shell for name in $(canonical_patches); do                \
                            if [ -x "$(VERSIONED_DIR_PATCH_APPLY)/$$name"   ] &&   \
                               [ -x "$(VERSIONED_DIR_PATCH_UNPATCH)/$$name" ];     \
                               then echo "$(VERSIONED_DIR_PATCH_APPLY)/$$name";    \
                            elif [ -x "$(VERSIONED_ALL_PATCH_APPLY)/$$name"   ] && \
                                 [ -x "$(VERSIONED_ALL_PATCH_UNPATCH)/$$name" ];   \
                               then echo "$(VERSIONED_ALL_PATCH_APPLY)/$$name";    \
                            elif [ -x "$(DIR_PATCH_APPLY)/$$name"   ] &&           \
                                 [ -x "$(DIR_PATCH_UNPATCH)/$$name" ]; then        \
                               echo "$(DIR_PATCH_APPLY)/$$name";                   \
                            elif [ -x "$(ALL_PATCH_APPLY)/$$name"   ] &&           \
                                 [ -x "$(ALL_PATCH_UNPATCH)/$$name" ]; then        \
                               echo "$(ALL_PATCH_APPLY)/$$name";                   \
                            else                                                 \
                               echo "$$name.error";                                \
                            fi;                                                  \
                        done)

temp_patch_not_found = $(filter %.error, $(temp_valid_patches))
patch_not_found = $(subst .error,,$(temp_patch_not_found))
ifneq ($(strip $(patch_not_found)),)
$(error Could not find patch for $(patch_not_found))
endif

valid_patches = $(filter-out %.error, $(temp_valid_patches))

ifeq ($(strip $(valid_patches)),)
$(error Could not find patch scripts for $(canonical_patches))
endif



canonical_unpatches = $(shell new="";                                         \
                              for name in $(canonical_patches); do            \
                                  new="$$name $$new";                         \
                              done;                                           \
                              echo $$new;)

temp_valid_unpatches = $(shell for name in $(canonical_unpatches); do            \
                            if [ -x "$(VERSIONED_DIR_PATCH_APPLY)/$$name"   ] &&   \
                               [ -x "$(VERSIONED_DIR_PATCH_UNPATCH)/$$name" ];     \
                              then echo "$(VERSIONED_DIR_PATCH_UNPATCH)/$$name";   \
                            elif [ -x "$(VERSIONED_ALL_PATCH_APPLY)/$$name"   ] && \
                                 [ -x "$(VERSIONED_ALL_PATCH_UNPATCH)/$$name" ];   \
                              then echo "$(VERSIONED_ALL_PATCH_UNPATCH)/$$name";   \
                            elif [ -x "$(DIR_PATCH_APPLY)/$$name"   ] &&           \
                                 [ -x "$(DIR_PATCH_UNPATCH)/$$name" ]; then        \
                               echo "$(DIR_PATCH_UNPATCH)/$$name";                 \
                            elif [ -x "$(ALL_PATCH_APPLY)/$$name"   ] &&           \
                                 [ -x "$(ALL_PATCH_UNPATCH)/$$name" ]; then        \
                               echo "$(ALL_PATCH_UNPATCH)/$$name";                 \
                            else                                                 \
                               echo $$name.error;                                \
                            fi;                                                  \
                        done)
temp_unpatch_not_found = $(filter %.error, $(temp_valid_unpatches))
unpatch_not_found = $(subst .error,,$(temp_unpatch_not_found))
ifneq ($(strip $(unpatch_not_found)),)
$(error Could not find unpatch for $(unpatch_not_found))
endif

valid_unpatches = $(filter-out %.error, $(temp_valid_unpatches))

ifeq ($(strip $(valid_unpatches)),)
$(error Could not find un-patch scripts for $(canonical_unpatches))
endif


endif
else
# OK. We want to patch the kernel, but there are no patches specified.
valid_patches = $(shell if [ -n "$(VERSIONED_PATCH_DIR)" ] &&                 \
                           [ -n "$(VERSIONED_DIR_PATCH_APPLY)" ] &&           \
                           [ -d "$(VERSIONED_DIR_PATCH_APPLY)" ]; then        \
                               run-parts --test $(VERSIONED_DIR_PATCH_APPLY); \
                        fi;                                                   \
                        if [ -n "$(VERSIONED_ALL_PATCH_DIR)" ] &&             \
                           [ -n "$(VERSIONED_ALL_PATCH_APPLY)" ] &&           \
                           [ -d "$(VERSIONED_ALL_PATCH_APPLY)" ]; then        \
                               run-parts --test $(VERSIONED_ALL_PATCH_APPLY); \
                        fi;                                                   \
                        if [ -n "$(PATCH_DIR)" ] &&                           \
                           [ -n "$(DIR_PATCH_APPLY)" ] &&                     \
                           [ -d "$(DIR_PATCH_APPLY)" ]; then                  \
                              run-parts --test $(DIR_PATCH_APPLY);            \
                        fi;                                                   \
                        if [ -n "$(ALL_PATCH_DIR)" ] &&                       \
                           [ -n "$(ALL_PATCH_APPLY)" ] &&                     \
                           [ -d "$(ALL_PATCH_APPLY)"  ]; then                 \
                              run-parts --test $(ALL_PATCH_APPLY);            \
                        fi)
valid_unpatches = $(shell ( if [ -n "$(VERSIONED_PATCH_DIR)"       ]  &&          \
                               [ -n "$(VERSIONED_DIR_PATCH_UNPATCH)" ] &&         \
                               [ -d "$(VERSIONED_DIR_PATCH_UNPATCH)" ]; then      \
                                 run-parts --test $(VERSIONED_DIR_PATCH_UNPATCH); \
                            fi;                                                   \
                            if [ -n "$(VERSIONED_ALL_PATCH_DIR)"    ]  &&         \
                               [ -n "$(VERSIONED_ALL_PATCH_UNPATCH)" ] &&         \
                               [ -d "$(VERSIONED_ALL_PATCH_UNPATCH)"  ]; then     \
                                 run-parts --test $(VERSIONED_ALL_PATCH_UNPATCH); \
                            fi;                                                   \
                            if [ -n "$(PATCH_DIR)"       ]  &&                    \
                               [ -n "$(DIR_PATCH_UNPATCH)" ] &&                   \
                               [ -d "$(DIR_PATCH_UNPATCH)" ]; then                \
                                 run-parts --test $(DIR_PATCH_UNPATCH);           \
                            fi;                                                   \
                            if [ -n "$(ALL_PATCH_DIR)"    ]  &&                   \
                               [ -n "$(ALL_PATCH_UNPATCH)" ] &&                   \
                               [ -d "$(ALL_PATCH_UNPATCH)"  ]; then               \
                                run-parts --test $(ALL_PATCH_UNPATCH);            \
                            fi) | tac)
endif
endif

old_applied_patches=$(shell if [ -f applied_patches ]; then                   \
                               cat applied_patches;                           \
                            else                                              \
                               echo '';                                       \
                            fi )

ifeq ($(strip $(valid_unpatches)),)
ifneq ($(strip $(old_applied_patches)),)
old_unpatches=$(shell new="";                                          \
                      for name in $(notdir $(old_applied_patches)); do \
                          new="$$name $$new";                          \
                      done;                                            \
                      echo $$new;)
temp_old_unpatches = $(shell for name in $(old_unpatches); do         \
                            if [ -x "$(VERSIONED_DIR_PATCH_UNPATCH)/$$name" ];  \
                              then echo "$(VERSIONED_DIR_PATCH_UNPATCH)/$$name";\
                            elif [ -x "$(VERSIONED_ALL_PATCH_UNPATCH)/$$name" ];\
                              then echo "$(VERSIONED_ALL_PATCH_UNPATCH)/$$name";\
                            elif [ -x "$(DIR_PATCH_UNPATCH)/$$name" ]; then     \
                               echo "$(DIR_PATCH_UNPATCH)/$$name";              \
                            elif [ -x "$(ALL_PATCH_UNPATCH)/$$name" ]; then     \
                               echo "$(ALL_PATCH_UNPATCH)/$$name";              \
                            else                                              \
                               echo "$$name.error";                             \
                            fi;                                               \
                        done)
temp_old_unpatch_not_found = $(filter %.error, $(temp_old_unpatches))
old_unpatch_not_found = $(subst .error,,$(temp_unpatch_not_found))
valid_unpatches = $(filter-out %.error, $(temp_old_unpatches))
endif
endif

