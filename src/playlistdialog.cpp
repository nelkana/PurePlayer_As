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
#include "playlistdialog.h"
#include "playlist.h"
#include "commonlib.h"

PlaylistDialog::PlaylistDialog(PlaylistModel* model, QWidget* parent) : QDialog(parent)
{
    setupUi(this);
    setAcceptDrops(true);

    _buttonPlay->hide();
    connect(_buttonAdd,    SIGNAL(clicked()), this, SLOT(buttonAdd_clicked()));
    connect(_buttonRemove, SIGNAL(clicked()), this, SLOT(buttonRemove_clicked()));
    connect(_buttonPrev,   SIGNAL(clicked()), this, SIGNAL(playPrev()));
    connect(_buttonNext,   SIGNAL(clicked()), this, SIGNAL(playNext()));

    _buttonUp->hide();
    _buttonDown->hide();
//  connect(_buttonUp,     SIGNAL(clicked()), this, SLOT(buttonUp_clicked()));
//  connect(_buttonDown,   SIGNAL(clicked()), this, SLOT(buttonDown_clicked()));

    _view = new PlaylistView(this);
    _verticalLayout->insertWidget(0, _view);
    connect(_view, SIGNAL(doubleClicked(const QModelIndex&)),
            this,  SLOT(view_doubleClicked(const QModelIndex&)));
    connect(_view, SIGNAL(pressedReturnKey(const QModelIndex&)),
            this,  SLOT(view_doubleClicked(const QModelIndex&)));

    _model = NULL;
    setModel(model);
}

void PlaylistDialog::buttonAdd_clicked()
{
    QString file = CommonLib::getOpenFileNameDialog(this, tr("ファイルを選択"));

    if( !file.isEmpty() )
         _model->appendTrack(file);
}

void PlaylistDialog::buttonRemove_clicked()
{
    PlaylistModel::Track* current = _model->currentTrack();

    QModelIndexList indexes = _view->selectionModel()->selectedRows();
    _model->removeRows(indexes);

    if( current != _model->currentTrack() )
        emit stopCurrentTrack();
}

void PlaylistDialog::buttonUp_clicked()
{
    _model->upCurrentTrackIndex();
}

void PlaylistDialog::buttonDown_clicked()
{
    _model->downCurrentTrackIndex();
}

void PlaylistDialog::view_doubleClicked(const QModelIndex& index)
{
    _model->setCurrentTrackIndex(index);
    emit playStopCurrentTrack();
}

void PlaylistDialog::setModel(PlaylistModel* model)
{
    if( model == NULL ) return;

    if( _model != NULL )
        disconnect(_buttonLoop, SIGNAL(toggled(bool)), _model, SLOT(setLoopPlay(bool)));

    _model = model;

    QItemSelectionModel* m = _view->selectionModel();
    _view->setModel(_model);
    delete m;

    connect(_buttonLoop, SIGNAL(toggled(bool)), _model, SLOT(setLoopPlay(bool)));
}

