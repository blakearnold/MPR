build_arch	= ia64
header_arch	= $(build_arch)
asm_link	= $(build_arch)
defconfig	= defconfig
flavours	= itanium mckinley
build_image	= vmlinux
kernel_file	= $(build_image)
install_file	= vmlinuz
compress_file	= yes

loader		= elilo
