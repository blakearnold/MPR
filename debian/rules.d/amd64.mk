build_arch	= x86_64
header_arch	= $(build_arch)
defconfig	= defconfig
flavours	= generic server
build_image	= bzImage
kernel_file	= arch/$(build_arch)/boot/bzImage
install_file	= vmlinuz

do_debug_image	= true

loader		= grub

#custom_flavours	= rt
