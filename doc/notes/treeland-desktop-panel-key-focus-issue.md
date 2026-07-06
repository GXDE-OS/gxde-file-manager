# Treeland: 桌面文件重命名/ESC键失效

## 表象

在Treeland上使用`layer-shell-qt`将`gxde-desktop-panel`改造为Wayland程序，无法对桌面图标进行重命名，桌面面板不接受任何键盘输入。



## 原因分析

> **注意**: 以下提及的Treeland源码提交截止至`0d0a4c35c9b6efd8806302f493beececa849ed69`，提交标题为「Merge: 20cd1eb e8dd819」，上游为https://gitee.com/GXDE-OS/treeland.git。



在Treeland的源码中，`./src/core/shellhandler.cpp`的770 ~ 784行提及了关于`requestActive`的规定：

```C++
...
    } else if (wrapper->type() == SurfaceWrapper::Type::Layer) {
        connect(wrapper, &SurfaceWrapper::requestActive, this, [wrapper]() {
            auto layerSurface = qobject_cast<WLayerSurface *>(wrapper->shellSurface());
            if (layerSurface->keyboardInteractivity() == WLayerSurface::KeyboardInteractivity::None)
                return;
            /*
             * if LayerSurface's keyboardInteractivity is `OnDemand`, only allow `Overlay` layer
             * surface get keyboard focus, to avoid dock/dde-desktop grab keyboard focus When they
             * restart
             */
            if (layerSurface->layer() == WLayerSurface::LayerType::Overlay
                || layerSurface->keyboardInteractivity()
                    == WLayerSurface::KeyboardInteractivity::Exclusive)
                Helper::instance()->activateSurface(wrapper);
        });
...
```



其中，这里有一行：

```C++
if (layerSurface->keyboardInteractivity() ==
    	WLayerSurface::KeyboardInteractivity::None) {
    return;
}  // 为了方便阅读我格式化了一下
```



以下是对这个操作的注释：

> If `LayerSurface`'s `keyboardInteractivity` is `OnDemand`, only allow `Overlay` layer surface get keyboard focus, to avoid `dock`/`dde-desktop` grab keyboard focus When they restart.
>
> 
>
> *（我自己翻译的）*
>
> 如果`LayerSurface`的`keyboardInteractivity`为`OnDemand`，仅允许`Overlay`类型的表面获取键盘焦点。这么做的目的是防止`dde-dock`/`dde-desktop`在重启时误获取键盘焦点。



`gxde-desktop-panel`是一个正常的`LayerShellQt::Window::LayerBackground`类而不是`Overlay`类（设置`Background`是对的，不然桌面层级不对），根据上述注释，无法获取键盘焦点...



仅在Treeland上发现了这个情况，在Mutter上没有问题，目前针对Treeland做了一个补丁，请见本项目的`gxde-rename-interface-treeland`，仅Treeland下用这个子项目，其它WM均走`gxde-desktop-panel`的原逻辑。
