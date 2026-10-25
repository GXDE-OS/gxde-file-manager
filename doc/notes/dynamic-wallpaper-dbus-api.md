# 壁纸设置 DBus 接口（含视频动态壁纸）

gxde-desktop-panel 在会话总线上注册 `com.deepin.dde.desktop` 服务
（`/com/deepin/dde/desktop`，接口名同服务名），任何程序都可以通过它设置桌面壁纸，
包括视频动态壁纸。服务未运行时会被 dbus 自动激活
（`/usr/share/dbus-1/services/com.deepin.dde.desktop.service`）。

> 桌面右键"壁纸与屏保"打开的壁纸选择器(gxde-wallpaper-chooser)同样支持视频壁纸：
> 视频会出现在列表末尾（扫描 /usr/share/wallpapers 等常规壁纸目录），
> 点击即在桌面上实时预览播放，选择"Only desktop"并关闭选择器后生效；
> 视频项没有"Only lock screen"按钮（锁屏壁纸不支持视频）。

## 方法

### SetDynamicWallpaper(path: String) -> Bool

设置视频动态壁纸。`path` 支持本地绝对路径和 `file://` URL。

- 支持所有 GStreamer/FFmpeg 能解码的视频格式（mp4/webm/mkv/...）
- 视频会被自动拷贝到壁纸库（`~/.config/deepin/dde-daemon/appearance/custom-wallpapers/`），
  防止源文件在可移动设备上、拔出后丢失
- 设置会被持久化到 `~/.config/GXDE/dde-file-manager/dynamicWallpaper`，
  面板重启/重新登录后自动恢复播放
- `path` 传空串则停用动态壁纸，回退到静态壁纸
- 文件不存在、不可读或不是视频（MIME 不以 `video/` 开头）时返回 false
- 解码失败（文件损坏、缺解码器等）时自动回退到静态壁纸并保留配置

```bash
# 设置
gdbus call --session --dest com.deepin.dde.desktop \
    --object-path /com/deepin/dde/desktop \
    --method com.deepin.dde.desktop.SetDynamicWallpaper "/home/user/bg.mp4"
# 停用
gdbus call --session --dest com.deepin.dde.desktop \
    --object-path /com/deepin/dde/desktop \
    --method com.deepin.dde.desktop.SetDynamicWallpaper ""
```

### DynamicWallpaper() -> String

查询当前动态壁纸的完整路径；未启用时返回空串。

### SetWallpaper(path: String) -> Bool

设置静态壁纸（与壁纸选择器行为一致）：拷入壁纸库后同时写入
`com.deepin.daemon.Appearance` 守护进程与 GSettings
（`com.deepin.dde.appearance backgroundUris`）。设置成功后
动态壁纸会自动停用（回到静态壁纸语义）。

## 信号

### DynamicWallpaperChanged(path: String)

动态壁纸启用、更换或停用时发出。`path` 为空串表示已停用。
监听示例：

```bash
gdbus monitor --session --dest com.deepin.dde.desktop \
    --object-path /com/deepin/dde/desktop
```

## 注意事项

- 静态壁纸的缩放方式（壁纸显示方式）同样作用于视频帧；
  跨屏(BackgroundSpanned)模式下视频按单屏尺寸铺满，不做多屏画布拼接
- X11 与 Wayland（含 Treeland layer-shell）会话下均可使用
- 动态壁纸播放时静音、无限循环；面板被杀掉后 dbus 会按 .service 文件自动重新拉起
