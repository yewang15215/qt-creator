/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrysetuppage.h"
#include "blackberrysetupwidget.h"
#include "qnxconstants.h"

#include <QCoreApplication>

namespace Qnx {
namespace Internal {

BlackBerrySetupPage::BlackBerrySetupPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(Core::Id(Constants::QNX_BB_SETUP_ID));
    setDisplayName(tr("Setup"));
    setCategory(Constants::QNX_BB_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("BlackBerry", Constants::QNX_BB_CATEGORY_TR));
    setCategoryIcon(QLatin1String(Constants::QNX_BB_CATEGORY_ICON));
}

QWidget *BlackBerrySetupPage::widget()
{
    if (!m_widget)
        m_widget = new BlackBerrySetupWidget;
    return m_widget;
}

void BlackBerrySetupPage::apply()
{
}

void BlackBerrySetupPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace Qnx
