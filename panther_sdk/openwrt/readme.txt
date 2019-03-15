编译前的准备工作
1.执行 install_host_tools.sh
2.make menuconfig  选择自己的项目名称
3.make


#关于项目名称的设置：
1.openwrt/include/project.mk  中定义的项目名称只会在openwrt/Makefile中被用到，用于生成不同的固件名
2.openwrt/config/Config-project.in中定义项目名称会在代码中被使用，前提是在make menuconfig时选中了该项目名
3.想要增加新的项目名称，可在openwrt/config/Config-project.in中依照原有样例添加
