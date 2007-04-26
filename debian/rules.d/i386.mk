build_arch	= i386
header_arch	= x86_64
defconfig	= defconfig
flavours	= 386 generic server server-bigiron
build_image	= bzImage
kernel_file	= arch/$(build_arch)/boot/bzImage
install_file	= vmlinuz

do_debug_image	= true

loader		= grub

custom_flavours	= rt xen
