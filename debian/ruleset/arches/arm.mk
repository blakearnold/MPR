######################### -*- Mode: Makefile-Gmake -*- ########################
## arm.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Fri Dec  9 14:58:51 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Fri Dec  9 15:02:02 2005
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 3
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : 
## 
## arch-tag: ac6e7f1e-b138-49a9-b007-abec5154ccf2
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

### ARM
ifeq ($(strip $(architecture)),arm)
  GUESS_SUBARCH:='netwinder'

  ifneq (,$(findstring $(KPKG_SUBARCH),netwinder))
    KPKG_SUBARCH:=$(GUESS_SUBARCH)
    kimage := zImage
    target = Image
    kimagesrc = arch/$(KERNEL_ARCH)/boot/Image
    kimagedest = $(INT_IMAGE_DESTDIR)/vmlinuz-$(version)
    loaderdep=
    loader=nettrom
    loaderdoc=
    NEED_DIRECT_GZIP_IMAGE=NO
    DEBCONFIG= $(CONFDIR)/config.netwinder
  else
    kimage := zImage
    target = zImage
    NEED_DIRECT_GZIP_IMAGE=NO
    kimagesrc = arch/$(KERNEL_ARCH)/boot/zImage
    kimagedest = $(INT_IMAGE_DESTDIR)/vmlinuz-$(version)
    DEBCONFIG = $(CONFDIR)/config.arm
  endif
  kelfimagesrc = vmlinux
  kelfimagedest = $(INT_IMAGE_DESTDIR)/vmlinux-$(version)
endif

#Local variables:
#mode: makefile
#End:
