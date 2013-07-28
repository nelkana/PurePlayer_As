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
#include <QKeyEvent>
#include "commonmenu.h"

void CommonMenu::addNoCloseAction(QAction* a)
{
    if( _listNoCloseAction.contains(a) )
        return;

    _listNoCloseAction << a;
}

void CommonMenu::keyPressEvent(QKeyEvent* e)
{
    switch( e->key() ) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    {
        QAction* action = activeAction();
        if( _listNoCloseAction.contains(action) ) {
            action->trigger();
            return;
        }
        break;
    }
    }

    QMenu::keyPressEvent(e);
}

void CommonMenu::mouseReleaseEvent(QMouseEvent* e)
{
    if( e->button() == Qt::LeftButton ) {
        QAction* action = activeAction();
        if( _listNoCloseAction.contains(action) ) {
            action->trigger();
            return;
        }
    }

    QMenu::mouseReleaseEvent(e);
}

