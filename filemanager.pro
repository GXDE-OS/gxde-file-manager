TEMPLATE  = subdirs

include(./common/common.pri)

SUBDIRS  += \
    gxde-file-manager \
    gxde-file-manager-daemon \
    gxde-file-manager-lib \
    gxde-file-manager-plugins \
    gxde-dock-plugins\
    gxde-desktop-panel \
    gxde-wallpaper-chooser \
    dde-file-thumbnail-tool

isEqual(BUILD_MINIMUM, YES){

}else{
    SUBDIRS += usb-device-formatter
#    dde-advanced-property-plugin \
#    dde-sharefiles
}

# 关闭 deepin anything 编译
# SUBDIRS += deepin-anything-server-plugins
#!CONFIG(DISABLE_ANYTHING) {
#    message("Deepin Anything server plugin enabled for" $$ARCH)
#    SUBDIRS += deepin-anything-server-plugins
#}

gxde-file-manager.depends = gxde-file-manager-lib
gxde-dock-plugins.depends = gxde-file-manager-lib
gxde-desktop-panel.depends = gxde-file-manager-lib
gxde-file-manager-daemon.depends = gxde-file-manager-lib
#deepin-anything-server-plugins.depends = gxde-file-manager-lib
#dde-sharefiles.depends = gxde-file-manager-lib
