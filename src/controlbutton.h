/*  Copyright (C) 2012-2013 nel

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
#ifndef CONTROLBUTTON_H
#define CONTROLBUTTON_H

#include <QPushButton>
#include <QEvent>

class ControlButton : public QPushButton
{
    Q_OBJECT

public:
    ControlButton(QWidget* parent) : QPushButton(parent) { init(); }
    ControlButton(const QIcon& icon, const QString& text, QWidget* parent)
                                            : QPushButton(icon, text, parent) { init(); }

protected:
    bool event(QEvent*);

private:
    void init();
};

inline void ControlButton::init()
{
    setFocusPolicy(Qt::NoFocus);
}

inline bool ControlButton::event(QEvent* e)
{
    if( e->type() == QEvent::MouseMove )
        return true;

    return QPushButton::event(e);
}

#endif // CONTROLBUTTON_H

