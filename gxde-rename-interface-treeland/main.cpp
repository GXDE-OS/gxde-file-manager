/**
 * Copyright (C) 2026 CharOfString
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */


/**
 * 背景: Treeland下遇到了桌面文件重命名/ESC键失效的问题
 * 具体原因分析请见: ./doc/notes/treeland-desktop-panel-key-focus-issue.md
 * 打了补丁，当前Treeland下的重命名由子项目gxde-rename-interface-treeland负责
 * 其它WM的重命名依然走原版逻辑
 * ----------------------------------------------------------------------------
 * 用法: gxde-rename-interface-treeland <文件名>
 * ----------------------------------------------------------------------------
 * 原理: (仅限Treeland上) 用户点菜单重命名
 *   -> gxde-desktop-panel里的CanvasGridView::launchRenameHelper尝试拉起此对话框
 *   -> 此对话框读取用户输入，检测名称是否非法
 *   -> 若新文件名合法，把新文件名打印到stdout，退出
 *   -> gxde-desktop-panel里的CanvasGridView::launchRenameHelper读取stdout回传的新名字
 *   -> 获取到名字后，由gxde-desktop-panel使用DFileService::renameFile负责改名
 */

#include <QTextCodec>
#include <QTextStream>
#include <QLineEdit>
#include <QString>
#include <QFile>
#include <QLibraryInfo>

#include <DApplication>
#include <ddialog.h>
#include <dlineedit.h>

#include "waylandutils.h"

DWIDGET_USE_NAMESPACE

static DDialog* dialogGen = nullptr;
static DLineEdit* lineEditGen = nullptr;
static int okIndex = -1;

// 校验非法文件名: Linux 文件系统层面真正非法的是 '/' 和 NUL；再加上空、'.'、'..'。
// (NUL 无法在输入框里键入，不必单独处理。) 返回非法理由；合法则返回空 QString。
static QString getInvalidReason(const QString& name) {
    if (name.isEmpty()) {
        return QObject::tr("The file name can not be empty");
    } else if (name == "." || name == "..") {
        return QObject::tr("Invalid file name");
    } else if (name.contains(QLatin1Char('/'))) {
        return QObject::tr("The file name can not contain the \"/\" character");
    } else {
        return QString();
    }
}

// 尝试校验
static void tryRename() {
    const QString name = lineEditGen->text().trimmed();
    const QString reason = getInvalidReason(name);
    if (reason.isEmpty()) {
        dialogGen->done(okIndex);
    } else {
        lineEditGen->setAlert(true);
        lineEditGen->showAlertMessage(reason);
    }
}

// 处理onRename
static void onRenameButtonClicked(int index, const QString& text) {
    if (index == okIndex) {
        tryRename();
    } else {
        dialogGen->done(index);
    }
}

// 修改文本框文字时消除告警
static void onRenameTextChanged(const QString& text) {
    if (lineEditGen->isAlert()) {
        lineEditGen->setAlert(false);
        lineEditGen->hideAlertMessage();
    }
}

int main(int argc, char* argv[]) {
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

    // unset掉Wayland集成，这样才能在Treeland上拿键盘焦点，同时unset掉XWayland
    qunsetenv("QT_WAYLAND_SHELL_INTEGRATION");
    qunsetenv("DTK2_XWAYLAND");

    if (WaylandUtils::isWaylandSession()) {
        const QString currentDE = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();
        const QString dwaylandPlugin = QLibraryInfo::location(QLibraryInfo::PluginsPath)
            + QStringLiteral("/platforms/libdwayland.so");

        // 可以安全地假设GXDE总是支持DWayland
        if (currentDE.contains("deepin") || currentDE.contains("gxde")) {
            qputenv("QT_QPA_PLATFORM", "dwayland");
        } else if (QFile::exists(dwaylandPlugin)) {
            qputenv("QT_QPA_PLATFORM", "dwayland");
        } else {
            qputenv("QT_QPA_PLATFORM", "wayland");
        }
    }

    DApplication app(argc, argv);
    app.setOrganizationName("gxde");
    app.setApplicationName("gxde-rename-interface-treeland");
    app.loadTranslator();

    // 第一个参数是程序自身的名字，不用提了
    // 第二个参数是待重命名的文件名，这个必须要，所以参数必须至少等于2
    // 如果有第三个怎么办？就不应该存在，直接忽略...
    if (argc < 2) {
        return -1;
    }

    // 获取原文件名
    const QString curName = QString::fromLocal8Bit(argv[1]);

    DDialog dialog;
    dialog.setTitle(QObject::tr("Rename") + QStringLiteral("「%1」").arg(curName));
    dialog.setMessage(QObject::tr("Please keep the file extension"));

    DLineEdit* edit = new DLineEdit(&dialog);
    edit->setText(curName);
    edit->selectAll();
    dialog.addContent(edit);

    const int cancelIndex = dialog.addButton(QObject::tr("Cancel"), false,
        DDialog::ButtonWarning);
    const int okIdx = dialog.addButton(QObject::tr("Confirm"), true,
        DDialog::ButtonRecommend);
    Q_UNUSED(cancelIndex)

    // 按下按钮不自动关闭，由onRenameButtonClicked决定是否关闭
    dialog.setOnButtonClickedClose(false);
    dialogGen = &dialog;
    lineEditGen = edit;
    okIndex = okIdx;

    QObject::connect(&dialog, &DDialog::buttonClicked, onRenameButtonClicked);
    QObject::connect(edit, &QLineEdit::returnPressed, tryRename);
    QObject::connect(edit, &QLineEdit::textChanged, onRenameTextChanged);

    QMetaObject::invokeMethod(edit, "setFocus", Qt::QueuedConnection);
    QMetaObject::invokeMethod(edit, "selectAll", Qt::QueuedConnection);

    if (dialog.exec() == okIdx) {
        const QString name = edit->text().trimmed();
        if (!name.isEmpty()) {
            QTextStream out(stdout);
            out << name << "\n";
            out.flush();
            return 0;
        }
    }
    return 1;
}
