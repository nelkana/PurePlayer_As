/*  Copyright (C) 2013 nel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef COMMONMENU_H
#define COMMONMENU_H

#include <QMenu>

class QKeyEvent;

class CommonMenu : public QMenu
{
    Q_OBJECT

public:
    CommonMenu(QWidget* parent=0) : QMenu(parent) {}
    CommonMenu(const QString& title, QWidget* parent=0) : QMenu(title, parent) {}
    virtual ~CommonMenu() {}

    void addNoCloseAction(QAction*);
    void deleteNoCloseAction(QAction* a) { _listNoCloseAction.removeOne(a); }

protected:
    void keyPressEvent(QKeyEvent*);
    void mouseReleaseEvent(QMouseEvent*);

private:
    QList<QAction*> _listNoCloseAction;
};

#endif //COMMONMENU_H

