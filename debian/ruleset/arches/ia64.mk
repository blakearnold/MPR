######################### -*- Mode: Makefile-Gmake -*- ########################
## ia64.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 18:31:09 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Mon Oct 31 18:31:09 2005
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 0
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : handle the architecture specific variables.
## 
## arch-tag: c0f7cb04-37af-46f6-a46e-f10ff6887e3d
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

kimage := vmlinuz
loaderdep=elilo
loader=elilo
loaderdoc=
target=compressed
NEED_DIRECT_GZIP_IMAGE=NO
kimagesrc=vmlinux.gz
kimagedest=$(INT_IMAGE_DESTDIR)/vmlinuz-$(version)
kelfimagesrc = vmlinux
kelfimagedest = $(INT_IMAGE_DESTDIR)/vmlinux-$(version)
DEBCONFIG=$(CONFDIR)/config.$(KPKG_SUBARCH)

#Local variables:
#mode: makefile
#End:
