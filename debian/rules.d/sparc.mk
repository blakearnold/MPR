build_arch	= sparc64
header_arch	= $(build_arch)
asm_link	= $(build_arch)
defconfig	= defconfig
flavours	= sparc64 sparc64-smp
build_image	= image
kernel_file     = arch/$(build_arch)/boot/image
install_file	= vmlinuz
compress_file	= Yes

loader		= silo
