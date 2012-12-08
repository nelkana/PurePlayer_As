/*  Copyright (C) 2012 nel

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
#ifndef PLAYLISTDIALOG_H
#define PLAYLISTDIALOG_H

#include <QDialog>
#include "ui_playlistdialog.h"

class PlaylistModel;
class PlaylistView;

class PlaylistDialog : public QDialog, Ui::PlaylistDialog
{
    Q_OBJECT

public:
    explicit PlaylistDialog(PlaylistModel* model, QWidget* parent);

signals:
    void playStopCurrentTrack();
    void stopCurrentTrack();
    void playPrev();
    void playNext();

protected slots:
    void buttonAdd_clicked();
    void buttonRemove_clicked();
    void buttonUp_clicked();
    void buttonDown_clicked();

    void view_doubleClicked(const QModelIndex&);

private:
    void setModel(PlaylistModel* model);

    PlaylistModel* _model;
    PlaylistView*  _view;
};

#endif // PLAYLISTDIALOG_H

