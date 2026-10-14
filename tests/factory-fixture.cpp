// SPDX-License-Identifier: GPL-3.0-or-later
#include <QObject>
class FactoryFixture : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.gxde.tests.FactoryFixture" FILE "factory-fixture.json")
public:
    FactoryFixture() { setObjectName(QStringLiteral("factory-fixture")); }
};
#include "factory-fixture.moc"
