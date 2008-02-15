build_arch	= x86_64
header_arch	= $(build_arch)
asm_link	= x86
defconfig	= defconfig
flavours	= generic
ifeq ($(is_ppa_build),)
flavours	+= server
endif
build_image	= bzImage
kernel_file	= arch/$(build_arch)/boot/bzImage
install_file	= vmlinuz

do_debug_image	= true

loader		= grub

#
# No custom binaries for the PPA build.
#
ifeq ($(is_ppa_build),)
#custom_flavours	= xen rt
custom_flavours	= 
endif
