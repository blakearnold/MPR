build_arch	= x86_64
header_arch	= $(build_arch)
asm_link	= x86
defconfig	= defconfig
flavours	= generic
flavours	+= server
build_image	= bzImage
kernel_file	= arch/$(build_arch)/boot/bzImage
install_file	= vmlinuz

do_debug_image	= true

loader		= grub

#
# No custom binaries for the PPA build.
#
custom_flavours	= rt xen openvz
