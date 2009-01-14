/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef ACTIONCONTAINER_H
#define ACTIONCONTAINER_H

#include <QtCore/QObject>
#include <QtGui/QMenu>
#include <QtGui/QToolBar>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>

namespace Core {

class Command;

class ActionContainer : public QObject
{
public:
    enum EmptyAction {
        EA_Mask             = 0xFF00,
        EA_None             = 0x0100,
        EA_Hide             = 0x0200,
        EA_Disable          = 0x0300
    };

    virtual void setEmptyAction(EmptyAction ea) = 0;

    virtual int id() const = 0;

    virtual QMenu *menu() const = 0;
    virtual QMenuBar *menuBar() const = 0;

    virtual QAction *insertLocation(const QString &group) const = 0;
    virtual void appendGroup(const QString &group) = 0;
    virtual void addAction(Core::Command *action, const QString &group = QString()) = 0;
    virtual void addMenu(Core::ActionContainer *menu, const QString &group = QString()) = 0;

    virtual bool update() = 0;
    virtual ~ActionContainer() {}
};

} // namespace Core

#endif // ACTIONCONTAINER_H
