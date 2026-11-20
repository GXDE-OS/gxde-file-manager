/*
 * Copyright (C) 2026 GXDE OS <https://gxde.top/>
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
#include "dfmfilepreviewplugin.h"

#include "debpreview.h"

DFM_BEGIN_NAMESPACE

class DebPreviewPlugin : public DFMFilePreviewPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DFMFilePreviewFactoryInterface_iid FILE "dde-deb-preview-plugin.json")

public:
    DFMFilePreview *create(const QString &key) Q_DECL_OVERRIDE
    {
        Q_UNUSED(key)

        return new DebPreview();
    }
};

DFM_END_NAMESPACE

#include "main.moc"
