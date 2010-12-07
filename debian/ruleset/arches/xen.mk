######################### -*- Mode: Makefile-Gmake -*- ########################
## xen.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 18:29:36 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Mon Oct 31 18:29:36 2005
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 0
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : handle the architecture specific variables.
## 
## arch-tag: 84291754-05f6-4240-b70b-45084bc51524
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

KERNEL_ARCH = xen
architecture = i386

ifeq (,$(findstring $(KPKG_SUBARCH),xen0 xenu))
     KPKG_SUBARCH:=xen0
endif
DEBCONFIG = $(CONFDIR)/config.$(KPKG_SUBARCH)

ifneq ($(shell if [ $(VERSION)  -ge  2 ]  && [ $(PATCHLEVEL) -ge 5 ] &&    \
                  [ $(SUBLEVEL) -ge 41 ]; then echo new;                   \
             elif [ $(VERSION)  -ge  2 ]  && [ $(PATCHLEVEL) -ge 6 ]; then \
                                          echo new;                        \
             elif [ $(VERSION)  -ge  3 ]; then echo new; fi),)
  target    = vmlinuz
else
  target    = bzImage
endif
kimage := $(target)

ifeq (,$(filter xen0,$(KPKG_SUBARCH)))
   # only domain-0 are bootable via xen so only domain0 subarch needs grub and xen-vm
   loaderdep=grub,xen-vm
   loader=grub
   loaderdoc=
else
   loaderdep=
   loader=
   loaderdoc=
endif

kimagesrc = $(kimage)
kimagedest = $(INT_IMAGE_DESTDIR)/xen-linux-$(version)

#Local variables:
#mode: makefile
#End:
