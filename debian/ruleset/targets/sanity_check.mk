######################### -*- Mode: Makefile-Gmake -*- ########################
## sanity_check.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Wed Nov  2 08:36:52 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Wed Nov  2 08:36:52 2005
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 0
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : This contains a sanity check that must be passed
##                     before creating a kernel package
## 
## arch-tag: ea49388f-8cfb-49e7-9c0c-b62f7260a07b
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

sanity_check:
ifeq ($(strip $(IN_KERNEL_DIR)),)
	@echo "Not in correct source directory"
	@echo "You should invoke this command from the top level directory of"
	@echo "a linux kernel source directory tree, and as far as I can tell,"
	@echo "the current directory:"
	@echo "	$(SRCTOP)"
	@echo "is not a top level linux kernel source directory. "
	@echo "" 
	@echo "	(If I am wrong then kernel-packages and the linux kernel" 
	@echo "	 are so out sync that you'd better get the latest versions" 
	@echo "	 of the kernel-package package and the Linux sources)" 
	@echo "" 
	@echo "Please change directory to wherever linux kernel sources" 
	@echo "reside and try again." 
	exit 1 
endif 
ifneq ($(strip $(HAVE_VALID_PACKAGE_VERSION)),YES)
	@echo "Problems ecountered with the version number $(debian)."
	@echo "$(HAVE_VALID_PACKAGE_VERSION)"
	@echo ""
	@echo "Please re-read the README file and try again."
	exit 2
endif
ifeq ($(strip $(STOP_FOR__BIN86)),YES)
	@echo "You Need to install the package bin86 before you can "
	@echo "compile the kernel on this machine"
	@echo ""
	@echo "Please install bin86 and try again."
	exit 3
endif
ifneq ($(strip $(HAVE_VERSION_MISMATCH)),)
	@echo "The changelog says we are creating $(saved_version)"
	@echo "However, I thought the version is $(version)"
	exit 4
endif


#Local variables:
#mode: makefile
#End:
