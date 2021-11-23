sudo xcode-select -switch /Library/Developer/CommandLineTools

#### envsetup:




- hmm


#### 刷写设备

- make fastboot adb
- adb reboot bootloader
- fastboot flashall -w


#### adb 命令
- adb reboot
- adb reboot recovery
- adb reboot bootloader
- fastboot flash boot boot.img
- fastboot flash system system.img
- fastboot flash userdata userdata.img
- fastboot flash aboot emmc_appsboot.mbn 
- fastboot flash recovery recovery.img 
- fastboot flash cache cache.img
- fastboot flash mdtp mdtp.img
- fastboot flash persist persist.img
- fastboot reboot