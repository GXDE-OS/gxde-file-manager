// SPDX-License-Identifier: GPL-3.0-or-later
#include "dfmfactoryloader.h"
#include <QtTest>
DFM_USE_NAMESPACE
class ThreadProbe : public QObject
{
public:
    int value = 0;
    int read(int add) const { return value + add; }
    void write(int v) { value = v; }
    bool inOwnThread() const { return QThread::currentThread() == thread(); }
};
class FactoryLoaderSmoke : public QObject
{
    Q_OBJECT
private slots:
    void loadRefreshAndDestroy()
    {
        QCOMPARE(sizeof(DFMFactoryLoader), sizeof(QObject));
        QVERIFY2(!DFMGlobal::isRootUser(), "Run as a normal user: the application disables plugins for root.");
        for (int iteration = 0; iteration < 3; ++iteration) {
            DFMFactoryLoader loader("org.gxde.tests.FactoryFixture", {}, Qt::CaseInsensitive);
            QCOMPARE(loader.metaData().size(), 1);
            QCOMPARE(loader.keyMap().size(), 1);
            QCOMPARE(loader.thread(), QCoreApplication::instance()->thread());
            const int index = loader.indexOf("FIXTUREKEY");
            QVERIFY(index >= 0);
            QObject *plugin = loader.instance(index);
            QVERIFY(plugin);
            QCOMPARE(plugin->objectName(), QStringLiteral("factory-fixture"));
            QCOMPARE(plugin->thread(), QCoreApplication::instance()->thread());
            QVERIFY(loader.pluginLoader("fixturekey"));
            QCOMPARE(loader.pluginLoaderList("FIXTUREKEY").size(), 1);
            QVERIFY(!loader.instance(-1));
            QVERIFY(!loader.instance(999));
            DFMFactoryLoader::refreshAll();
            QCOMPARE(loader.metaData().size(), 1);
        }
        // refreshAll must not retain any of the destroyed loader instances.
        DFMFactoryLoader::refreshAll();
    }
    void threadDispatch()
    {
        ThreadProbe local;
        QCOMPARE(DThreadUtil::runInMainThread(&local, &ThreadProbe::read, 7), 7);
        DThreadUtil::runInMainThread(&local, &ThreadProbe::write, 9);
        QCOMPARE(local.value, 9);
        QCOMPARE(DThreadUtil::runInMainThread([] { return 12; }), 12);
        QThread worker;
        auto *probe = new ThreadProbe;
        probe->moveToThread(&worker);
        connect(&worker, &QThread::finished, probe, &QObject::deleteLater);
        worker.start();
        DThreadUtil::runInThread(&worker, probe, &ThreadProbe::write, 40);
        const ThreadProbe *constant = probe;
        const int value = DThreadUtil::runInThread(&worker, constant, &ThreadProbe::read, 2);
        const bool correctThread = DThreadUtil::runInThread(&worker, constant, &ThreadProbe::inOwnThread);
        worker.quit();
        worker.wait();
        QCOMPARE(value, 42);
        QVERIFY(correctThread);
    }
    void repeatedKeys()
    {
        DFMFactoryLoader loader("org.gxde.tests.FactoryFixture", {}, Qt::CaseInsensitive, true);
        QCOMPARE(loader.metaData().size(), 2);
        QCOMPARE(loader.getAllIndexByKey("fixturekey").size(), 2);
        QCOMPARE(loader.pluginLoaderList("FIXTUREKEY").size(), 2);
        for (int index : loader.getAllIndexByKey("fixturekey"))
            QVERIFY(loader.instance(index));
        loader.update();
        QCOMPARE(loader.metaData().size(), 2);
    }
};
QTEST_MAIN(FactoryLoaderSmoke)
#include "factoryloader-smoke.moc"
