#
# Copyright (C) 2011 OpenWrt.org
#

PART_NAME=firmware
REQUIRE_IMAGE_METADATA=1

platform_check_image() {
	return 0
}

platform_do_upgrade() {
	local board=$(board_name)

	case "$board" in
	hpe,msm460|\
	ocedo,panda|\
	sophos,red-15w-rev1|\
	watchguard,firebox-t10)
		nand_do_upgrade "$1"
		;;
	watchguard,t30-w)
		watchguard_sdcard_do_upgrade "$1" uImage_t30 t30.dtb
		;;
	*)
		default_do_upgrade "$1"
		;;
	esac
}
