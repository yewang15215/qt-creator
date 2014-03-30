/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "vcproject.h"
#include "vcprojectbuildoptionspage.h"
#include "vcprojectmanager.h"
#include "vcprojectmanagerconstants.h"
#include "vcschemamanager.h"
#include "utils.h"
#include "vcprojectmodel/vcprojectdocument_constants.h"

#include <QtXmlPatterns/QXmlSchema>
#include <QtXmlPatterns/QXmlSchemaValidator>

using namespace ProjectExplorer;

namespace VcProjectManager {
namespace Internal {

VcManager::VcManager(VcProjectBuildOptionsPage *configPage) :
    m_configPage(configPage)
{}

QString VcManager::mimeType() const
{
    return QLatin1String(Constants::VCPROJ_MIMETYPE);
}

ProjectExplorer::Project *VcManager::openProject(const QString &fileName, QString *errorString)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    // Check whether the project file exists.
    if (canonicalFilePath.isEmpty()) {
        if (errorString)
            *errorString = tr("Failed opening project '%1': Project file does not exist").
                arg(QDir::toNativeSeparators(fileName));
        return 0;
    }

    // check if project is a valid vc project
    // versions supported are 2003, 2005 and 2008
    VcDocConstants::DocumentVersion docVersion = Utils::getProjectVersion(canonicalFilePath);

    if (docVersion != VcDocConstants::DV_UNRECOGNIZED)
        return new VcProject(this, canonicalFilePath, docVersion);

    qDebug() << "VcManager::openProject: Unrecognized file version";
    return 0;
}

void VcManager::updateContextMenu(Project *project, ProjectExplorer::Node *node)
{
    Q_UNUSED(node);
    m_contextProject = project;
}

} // namespace Internal
} // namespace VcProjectManager