#!/system/bin/sh
BASE_PATH=$(dirname $0)
while [ ! -e /sdcard/Android/CuBackgroundCtrl/config.txt ] ; do
	sleep 1
done
${BASE_PATH}/CuBackgroundCtrl -R "/sdcard/Android/CuBackgroundCtrl/config.txt" "/sdcard/Android/CuBackgroundCtrl/log.txt"
