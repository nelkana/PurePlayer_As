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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "ui_configdialog.h"
#include "configdata.h"

class ConfigDialog : public QDialog, Ui::ConfigDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget* parent);

    void setData(ConfigData::Data* data);

public slots:
    void ok() { hide(); apply(); }
    void apply();
    void setScreenshotPathFromDialog();
    void setMplayerPathFromDialog();

signals:
    void applied(bool requestRestartMplayer);

protected:
    void showEvent(QShowEvent*);

private slots:
    void checkBoxSoftVideoEqClicked(bool checked);

private:
    ConfigData::Data* _data;
};

#endif // CONFIGDIALOG_H

