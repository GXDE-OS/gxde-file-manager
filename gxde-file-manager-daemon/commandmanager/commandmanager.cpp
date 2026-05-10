/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *               2016 ~ 2018 dragondjf
 *
 * Author:     dragondjf<dingjiangfeng@deepin.com>
 *
 * Maintainer: dragondjf<dingjiangfeng@deepin.com>
 *             zccrs<zhangjide@deepin.com>
 *             Tangtong<tangtong@deepin.com>
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

#include "commandmanager.h"
#include "dbusadaptor/commandmanager_adaptor.h"
#include "../partman/command.h"
#include <QDBusConnection>
#include <QDBusVariant>
#include <QtConcurrent>
#include <QFuture>
#include <QDebug>


QString CommandManager::ObjectPath = "/com/deepin/filemanager/daemon/CommandManager";

CommandManager::CommandManager(QObject *parent) : QObject(parent)
{
    QDBusConnection::systemBus().registerObject(ObjectPath, this);
    m_commandManagerAdaptor = new CommandManagerAdaptor(this);
}
using namespace PartMan;

bool CommandManager::process(const QString &cmd, const QStringList &args,  QString &output,  QString &error)
{
    // 用C++11 Lambda表达式重写，改进可读性
    QFuture<bool> future = QtConcurrent::run([&] {
        return PartMan::SpawnCmd(cmd, args, output, error);
    });
    future.waitForFinished();
    bool ret = future.result();
    return ret;
}

bool CommandManager::startDetached(const QString &cmd, const QStringList &args)
{
    bool ret = QProcess::startDetached(cmd, args);
    qDebug() << ret << cmd << args;
    return ret;
}
