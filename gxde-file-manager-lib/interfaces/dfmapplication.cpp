/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "dfmapplication.h"
#include "private/dfmapplication_p.h"
#if QT_HAS_INCLUDE("anything_interface.h")
#include "anything_interface.h"
#else
#define DISABLE_QUICK_SEARCH
#endif

#include "dfmsettings.h"

#include <QCoreApplication>
#include <QMetaEnum>

DFM_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(DFMSettings, gsGlobal, ("deepin/gxde-file-manager", DFMSettings::GenericConfig))
Q_GLOBAL_STATIC_WITH_ARGS(DFMSettings, asGlobal, ("deepin/gxde-file-manager/gxde-file-manager", DFMSettings::GenericConfig))
//Q_GLOBAL_STATIC_WITH_ARGS(DFMSettings, asGlobal, ("gxde-file-manager", DFMSettings::AppConfig))

Q_GLOBAL_STATIC_WITH_ARGS(DFMSettings, gosGlobal, ("deepin/gxde-file-manager.obtusely", DFMSettings::GenericConfig))
Q_GLOBAL_STATIC_WITH_ARGS(DFMSettings, aosGlobal, ("deepin/gxde-file-manager/gxde-file-manager.obtusely", DFMSettings::GenericConfig))
//Q_GLOBAL_STATIC_WITH_ARGS(DFMSettings, aosGlobal, ("gxde-file-manager.obtusely", DFMSettings::AppConfig))

// blumia: since gxde-desktop-panel now also do show file selection dialog job, thus gxde-desktop-panel should share the same config file
//         with gxde-file-manager, so we use GenericConfig with specify path to simulate AppConfig.

DFMApplication *DFMApplicationPrivate::self = nullptr;

DFMApplicationPrivate::DFMApplicationPrivate(DFMApplication *qq)
{
    Q_ASSERT_X(!self, "DFMApplication", "there should be only one application object");
    self = qq;
}

void DFMApplicationPrivate::_q_onSettingsValueChanged(const QString &group, const QString &key, const QVariant &value, bool edited)
{
    if (group == QT_STRINGIFY(ApplicationAttribute)) {
        const QMetaEnum &me = QMetaEnum::fromType<DFMApplication::ApplicationAttribute>();

        DFMApplication::ApplicationAttribute aa = (DFMApplication::ApplicationAttribute)me.keyToValue(QByteArray("AA_" + key.toLatin1()).constData());

        if (edited)
            Q_EMIT self->appAttributeEdited(aa, value);

        Q_EMIT self->appAttributeChanged(aa, value);

        if (aa == DFMApplication::AA_IconSizeLevel) {
            Q_EMIT self->iconSizeLevelChanged(value.toInt());
        } else if (aa == DFMApplication::AA_ViewMode) {
            Q_EMIT self->viewModeChanged(value.toInt());
        }
    } else if (group == QT_STRINGIFY(GenericAttribute)) {
        const QMetaEnum &me = QMetaEnum::fromType<DFMApplication::GenericAttribute>();

        DFMApplication::GenericAttribute ga = (DFMApplication::GenericAttribute)me.keyToValue(QByteArray("GA_" + key.toLatin1()).constData());

        if (edited)
            Q_EMIT self->genericAttributeEdited(ga, value);

        Q_EMIT self->genericAttributeChanged(ga, value);

        switch (ga) {
        case DFMApplication::GA_PreviewDocumentFile:
        case DFMApplication::GA_PreviewImage:
        case DFMApplication::GA_PreviewTextFile:
        case DFMApplication::GA_PreviewVideo:
            Q_EMIT self->previewAttributeChanged(ga, value.toBool());
            break;
        case DFMApplication::GA_ShowedHiddenFiles:
            Q_EMIT self->showedHiddenFilesChanged(value.toBool());
            break;
        case DFMApplication::GA_ShowRecentFileEntry:
            Q_EMIT self->recentDisplayChanged(value.toBool());
            break;
        case DFMApplication::GA_PreviewCompressFile:
            Q_EMIT self->previewCompressFileChanged(value.toBool());
            break;
        case DFMApplication::GA_ShowCsdCrumbBarClickableArea:
            Q_EMIT self->csdClickableAreaAttributeChanged(value.toBool());
            break;
        default:
            break;
        }
    }
}

void DFMApplicationPrivate::_q_onSettingsValueEdited(const QString &group, const QString &key, const QVariant &value)
{
    _q_onSettingsValueChanged(group, key, value, true);
}

DFMApplication::DFMApplication(QObject *parent)
    : DFMApplication(new DFMApplicationPrivate(this), parent)
{
    qRegisterMetaType<ApplicationAttribute>();
    qRegisterMetaType<GenericAttribute>();
}

DFMApplication::~DFMApplication()
{

}

QVariant DFMApplication::appAttribute(DFMApplication::ApplicationAttribute aa)
{
    const QString group(QT_STRINGIFY(ApplicationAttribute));
    const QMetaEnum &me = QMetaEnum::fromType<ApplicationAttribute>();
    const QString key = QString::fromLatin1(me.valueToKey(aa)).split("_").last();

    return appSetting()->value(group, key);
}

DUrl DFMApplication::appUrlAttribute(DFMApplication::ApplicationAttribute aa)
{
    const QString group(QT_STRINGIFY(ApplicationAttribute));
    const QMetaEnum &me = QMetaEnum::fromType<ApplicationAttribute>();
    const QString key = QString::fromLatin1(me.valueToKey(aa)).split("_").last();

    return appSetting()->urlValue(group, key);
}

void DFMApplication::setAppAttribute(DFMApplication::ApplicationAttribute aa, const QVariant &value)
{
    const QString group(QT_STRINGIFY(ApplicationAttribute));
    const QMetaEnum &me = QMetaEnum::fromType<ApplicationAttribute>();
    const QString key = QString::fromLatin1(me.valueToKey(aa)).split("_").last();

    appSetting()->setValue(group, key, value);
}

bool DFMApplication::syncAppAttribute()
{
    return appSetting()->sync();
}

#ifndef DISABLE_QUICK_SEARCH
static ComDeepinAnythingInterface *getAnythingInterface()
{
    static ComDeepinAnythingInterface *interface = new ComDeepinAnythingInterface("com.deepin.anything", "/com/deepin/anything", QDBusConnection::systemBus());

    return interface;
}
#endif

QVariant DFMApplication::genericAttribute(DFMApplication::GenericAttribute ga)
{
    if (ga == GA_IndexInternal) {
#ifndef DISABLE_QUICK_SEARCH
        return getAnythingInterface()->autoIndexInternal();
#else
        return false;
#endif
    } else if (ga == GA_IndexExternal) {
#ifndef DISABLE_QUICK_SEARCH
        return getAnythingInterface()->autoIndexExternal();
#else
        return false;
#endif
    }

    const QString group(QT_STRINGIFY(GenericAttribute));
    const QMetaEnum &me = QMetaEnum::fromType<GenericAttribute>();
    const QString key = QString::fromLatin1(me.valueToKey(ga)).split("_").last();

    return genericSetting()->value(group, key);
}

void DFMApplication::setGenericAttribute(DFMApplication::GenericAttribute ga, const QVariant &value)
{
    if (ga == GA_IndexInternal) {
#ifndef DISABLE_QUICK_SEARCH
        return getAnythingInterface()->setAutoIndexInternal(value.toBool());
#else
        return;
#endif
    } else if (ga == GA_IndexExternal) {
#ifndef DISABLE_QUICK_SEARCH
        return getAnythingInterface()->setAutoIndexExternal(value.toBool());
#else
        return;
#endif
    }

    const QString group(QT_STRINGIFY(GenericAttribute));
    const QMetaEnum &me = QMetaEnum::fromType<GenericAttribute>();
    const QString key = QString::fromLatin1(me.valueToKey(ga)).split("_").last();

    genericSetting()->setValue(group, key, value);
}

bool DFMApplication::syncGenericAttribute()
{
    return genericSetting()->sync();
}

DFMApplication *DFMApplication::instance()
{
    return DFMApplicationPrivate::self;
}

DFMSettings *DFMApplication::genericSetting()
{
    if (!gsGlobal.exists()) {
        if (instance()) {
            gsGlobal->moveToThread(instance()->thread());
            connect(gsGlobal, &DFMSettings::valueChanged,
                    instance(), &DFMApplication::onSettingsValueChanged);
            connect(gsGlobal, &DFMSettings::valueEdited,
                    instance(), &DFMApplication::onSettingsValueEdited);
        }

        gsGlobal->setAutoSync(true);
#ifndef DFM_NO_FILE_WATCHER
        gsGlobal->setWatchChanges(true);
#endif

        if (instance())
            Q_EMIT instance()->genericSettingCreated(gsGlobal);
    }

    return gsGlobal;
}

DFMSettings *DFMApplication::appSetting()
{
    if (!asGlobal.exists()) {
        if (instance()) {
            asGlobal->moveToThread(instance()->thread());
            connect(asGlobal, &DFMSettings::valueChanged,
                    instance(), &DFMApplication::onSettingsValueChanged);
            connect(asGlobal, &DFMSettings::valueEdited,
                    instance(), &DFMApplication::onSettingsValueEdited);
        }

        asGlobal->setAutoSync(true);
#ifndef DFM_NO_FILE_WATCHER
        asGlobal->setWatchChanges(true);
#endif

        if (instance())
            Q_EMIT instance()->appSettingCreated(asGlobal);
    }

    return asGlobal;
}

DFMSettings *DFMApplication::genericObtuselySetting()
{
    if (!gosGlobal.exists()) {
        gosGlobal->setAutoSync(false);
#ifndef DFM_NO_FILE_WATCHER
        gosGlobal->setWatchChanges(false);
#endif
    }

    return gosGlobal;
}

DFMSettings *DFMApplication::appObtuselySetting()
{
    if (!aosGlobal.exists()) {
        aosGlobal->setAutoSync(false);
#ifndef DFM_NO_FILE_WATCHER
        aosGlobal->setWatchChanges(true);
#endif
    }

    return aosGlobal;
}

DFMApplication::DFMApplication(DFMApplicationPrivate *dd, QObject *parent)
    : QObject(parent)
    , d_ptr(dd)
{
    if (gsGlobal.exists()) {
        gsGlobal->moveToThread(thread());
        connect(gsGlobal, &DFMSettings::valueChanged,
                this, &DFMApplication::onSettingsValueChanged);
    }

    if (asGlobal.exists()) {
        asGlobal->moveToThread(thread());
        connect(asGlobal, &DFMSettings::valueChanged,
                this, &DFMApplication::onSettingsValueChanged);
    }
}

void DFMApplication::onSettingsValueChanged(const QString &group, const QString &key, const QVariant &value)
{
    d_func()->_q_onSettingsValueChanged(group, key, value);
}

void DFMApplication::onSettingsValueEdited(const QString &group, const QString &key, const QVariant &value)
{
    d_func()->_q_onSettingsValueEdited(group, key, value);
}

DFM_END_NAMESPACE
