ui_print "- CuBackgroundCtrl Magisk Module"
ui_print "- Installing..."

ANDROID_SDK=$(getprop ro.build.version.sdk)
KERNEL_VERSION=$(uname -r)
DEVICE_ABI=$(getprop ro.product.cpu.abi)
if [ ${ANDROID_SDK} -lt 28 || ${KERNEL_VERSION%%.*} -lt 4 || ${DEVICE_ABI} != "arm64-v8a" ] ; then
	abort "- Your device does not meet the requirement, Abort."
fi

unzip -o "${ZIPFILE}" -x 'META-INF/*' -d ${MODPATH} >&2
chmod -R 7777 ${MODPATH}

if [ ! -d /sdcard/Android/CuBackgroundCtrl ] ; then
	mkdir -p /sdcard/Android/CuBackgroundCtrl
	cp -f ${MODPATH}/config.txt /sdcard/Android/CuBackgroundCtrl/config.txt
	ui_print "- Config file released at '/sdcard/Android/CuBackgroundCtrl/config.txt'"
fi
rm -f ${MODPATH}/config.txt

ui_print "- Install finished."
