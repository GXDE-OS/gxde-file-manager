#!/usr/bin/env python3
"""Isolated grid-profile regression test; requires a C++17 compiler and Qt6Core.

Compile the production GridManagerPrivate implementation with lightweight file-info
and queued configuration adapters. No desktop session or user settings are touched.
Run with: python3 tests/test-gridmanager-profiles.py
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

ADAPTERS = r'''
struct FakeInfo { QString id; FakeInfo(QString value):id(value){} QUrl fileUrl() const { return QUrl(id); } };
using DAbstractFileInfoPointer = QSharedPointer<FakeInfo>;
class Config {
public:
 inline static const QString groupGeneral="GeneralConfig", keyProfile="Profile", keyAutoAlign="AutoSort", keyAutoMerge="AutoMerge", keyHideIcon="HideIcon";
 QTemporaryDir dir;
 QSettings config{dir.filePath("desktop.conf"),QSettings::IniFormat};
 static Config *instance() { static Config c; return &c; }
 QSettings *settings() { return &config; }
};
class Presenter {
public:
 static Presenter *instance() { static Presenter p; return &p; }
 QList<std::function<void()>> pending;
 void flush() { auto tasks=pending; pending.clear(); for (auto task : tasks) task(); Config::instance()->settings()->sync(); }
 void removeConfig(QString g,QString k) { pending.append([=] { auto s=Config::instance()->settings(); s->beginGroup(g); s->remove(k); s->endGroup(); }); }
 void setConfigList(QString g,QStringList keys,QVariantList vals) { pending.append([=] { auto s=Config::instance()->settings(); s->beginGroup(g); for(int i=0;i<keys.size();++i) s->setValue(keys[i],vals[i]); s->endGroup(); }); }
};
'''

CASES = r'''
void check(bool ok, const char *message) { if (!ok) qFatal("FAIL: %s", message); }
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv);
 auto s=Config::instance()->settings();
 const QString item="file:///Desktop/example.desktop", added="file:///Desktop/new.desktop";
 auto seed = [&] {
   Presenter::instance()->flush();
   s->clear();
   s->setValue("GeneralConfig/Profile","Position_21x9");
   s->setValue("Position_21x10/1_0",item);
   s->setValue("Position_21x9/4_3",item);
   s->sync();
 };
 auto load = [&](GridManagerPrivate &grid, QStringList ids) {
   QList<DAbstractFileInfoPointer> files;
   for (auto id : ids) files << DAbstractFileInfoPointer::create(id);
   grid.createProfile(); grid.loadProfile(files);
 };
 seed();
 {
   GridManagerPrivate grid;
   grid.updateGridSize(21,10); load(grid,{item}); grid.updateGridSize(21,9);
   check(grid.m_itemGrids.value(item)==QPoint(4,3),"startup restores saved target position");
   check(s->value("Position_21x9/4_3").toString()==item,"saved position survives startup");
   grid.remove(QPoint(4,3),item); grid.add(QPoint(7,5),item); grid.syncProfile();
   grid.updateGridSize(21,10); grid.updateGridSize(21,9);
   check(grid.m_itemGrids.value(item)==QPoint(7,5),"round trip keeps latest manual move");
 }
 {
   Presenter::instance()->flush();
   GridManagerPrivate restarted;
   restarted.updateGridSize(21,10); load(restarted,{item}); restarted.updateGridSize(21,9);
   check(restarted.m_itemGrids.value(item)==QPoint(7,5),"restart keeps latest manual move");
 }
 seed();
 {
   GridManagerPrivate grid;
   grid.updateGridSize(21,10); grid.updateGridSize(21,9);
   check(s->contains("Position_21x9/4_3"),"pre-load resize preserves settings");
   load(grid,{item,added});
   check(grid.m_itemGrids.value(item)==QPoint(4,3),"pre-load resize restores position");
   check(grid.m_itemGrids.contains(added),"new file is placed");
   grid.remove(grid.m_itemGrids.value(item),item); grid.syncProfile();
   grid.updateGridSize(21,10); grid.updateGridSize(21,9);
   check(!grid.m_itemGrids.contains(item),"deleted file is not resurrected");
   grid.updateGridSize(22,10);
   check(grid.m_itemGrids.contains(added),"new geometry retains files");
   grid.updateGridSize(0,0);
   check(grid.coordWidth==22 && grid.coordHeight==10,"empty configure ignored");
 }
 seed();
 s->setValue("Position_21x9/999_999",added); s->sync();
 {
   GridManagerPrivate grid; grid.updateGridSize(21,9); load(grid,{item,added});
   check(grid.isValid(grid.m_itemGrids.value(added)),"out-of-bounds saved entry relocated");
   grid.autoArrange=true; grid.updateGridSize(21,10);
   check(grid.m_gridItems.contains(QPoint(0,0)),"automatic arrangement remains enabled");
   grid.autoMerge=true; grid.updateGridSize(21,9);
   check(grid.m_itemGrids.size()==2,"merge mode retains items");
 }
 seed();
 s->setValue("GeneralConfig/HideIcon",true); s->sync();
 {
   GridManagerPrivate grid; grid.updateGridSize(21,10); load(grid,{item});
   grid.updateGridSize(21,9); grid.syncProfile();
   check(s->contains("Position_21x9/4_3"),"hidden icons do not erase layout");
 }
 qInfo()<<"PASS: startup, restart, round trip, pre-load configure, file changes, new/invalid geometry, invalid coordinates, auto arrangement, merge and hidden icons";
}
'''


def main():
    source = (ROOT / "gxde-desktop-panel/presenter/gridmanager.cpp").read_text()
    implementation = source[source.index("class GridManagerPrivate"):
                            source.index("GridManager::GridManager()")]
    core = (ROOT / "gxde-desktop-panel/presenter/gridcore.h").read_text()
    helpers = core[core.index("inline QString positionKey"):
                   core.index("// TODO: move all grid")]
    flags = shlex.split(subprocess.check_output(
        ["pkg-config", "--cflags", "--libs", "Qt6Core"], text=True))
    with tempfile.TemporaryDirectory(prefix="gxde-grid-test-") as directory:
        path = Path(directory)
        cpp = path / "test.cpp"
        cpp.write_text("#include <QtCore>\n#include <atomic>\n#include <algorithm>\n"
                       + helpers + ADAPTERS + implementation + CASES)
        binary = path / "test"
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++17",
                        "-fPIC", str(cpp), "-o", str(binary), *flags], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
