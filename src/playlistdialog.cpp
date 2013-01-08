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
#include <QMenu>
#include "playlistdialog.h"
#include "commonlib.h"

PlaylistDialog::PlaylistDialog(PlaylistModel* model, QWidget* parent) : QDialog(parent)
{
    setupUi(this);
    setAcceptDrops(true);

    _buttonAdd->hide();
    _buttonPlay->hide();
    connect(_buttonAdd,    SIGNAL(clicked()), this, SLOT(buttonAdd_clicked()));
    connect(_buttonRemove, SIGNAL(clicked()), this, SLOT(buttonRemove_clicked()));
    connect(_buttonPrev,   SIGNAL(clicked()), this, SIGNAL(playPrev()));
    connect(_buttonNext,   SIGNAL(clicked()), this, SIGNAL(playNext()));
    connect(_buttonSort,   SIGNAL(clicked()), this, SLOT(buttonSort_clicked()));

    _view = new PlaylistView(this);
    _verticalLayout->insertWidget(0, _view);
    connect(_view, SIGNAL(doubleClicked(const QModelIndex&)),
            this,  SLOT(view_doubleClicked(const QModelIndex&)));
    connect(_view, SIGNAL(pressedReturnKey(const QModelIndex&)),
            this,  SLOT(view_doubleClicked(const QModelIndex&)));

    _model = NULL;
    setModel(model);
}

void PlaylistDialog::addFiles()
{
    QStringList files = CommonLib::getOpenFileNamesDialog(this, tr("ファイルを追加"));

    if( !files.isEmpty() )
        _model->appendTracks(files);
}

void PlaylistDialog::addDirectories()
{
    QString dir = CommonLib::getExistingDirectoryDialog(this, tr("ディレクトリから追加"));

    if( !dir.isEmpty() )
        _model->appendTracks(QStringList() << dir);
}

void PlaylistDialog::buttonAdd_clicked()
{
    QAction actAddFile(tr("ファイルを追加"), 0);
    connect(&actAddFile, SIGNAL(triggered()), this, SLOT(addFiles()));
    QAction actAddDirectory(tr("ディレクトリから追加"), 0);
    connect(&actAddDirectory, SIGNAL(triggered()), this, SLOT(addDirectories()));
    QMenu menu;
    menu.addAction(&actAddFile);
    menu.addAction(&actAddDirectory);

    QPoint point = mapToGlobal(QPoint(_buttonAdd->x(), _buttonAdd->y()+_buttonAdd->height()));
    menu.exec(point);
}

void PlaylistDialog::buttonRemove_clicked()
{
    QAction actAll(tr("全て削除"), 0);
    connect(&actAll, SIGNAL(triggered()), _model, SLOT(removeAllRows()));
    QAction actSelect(tr("選択項目を削除"), 0);
    actSelect.setShortcut(tr("del"));
    connect(&actSelect, SIGNAL(triggered()), _view, SLOT(removeSelectedTrack()));
    QMenu menu;
    menu.addAction(&actAll);
    menu.addAction(&actSelect);

    QPoint point = mapToGlobal(QPoint(_buttonRemove->x(), _buttonRemove->y() -menu.fontMetrics().height()-8));
    menu.exec(point, &actSelect);
}

void PlaylistDialog::buttonSort_clicked()
{
    QAction actTitleAs(tr("タイトル昇順"), 0);
    connect(&actTitleAs, SIGNAL(triggered()), _view, SLOT(sortTitleAscending()));
    QAction actTitleDes(tr("タイトル降順"), 0);
    connect(&actTitleDes, SIGNAL(triggered()), _view, SLOT(sortTitleDescending()));
    QAction actTimeAs(tr("時間昇順"), 0);
    connect(&actTimeAs, SIGNAL(triggered()), _view, SLOT(sortTimeAscending()));
    QAction actTimeDes(tr("時間降順"), 0);
    connect(&actTimeDes, SIGNAL(triggered()), _view, SLOT(sortTimeDescending()));
    QAction actPathAs(tr("パス昇順"), 0);
    connect(&actPathAs, SIGNAL(triggered()), _view, SLOT(sortPathAscending()));
    QAction actPathDes(tr("パス降順"), 0);
    connect(&actPathDes, SIGNAL(triggered()), _view, SLOT(sortPathDescending()));

    QMenu menu;
    menu.addAction(&actTitleAs);
    menu.addAction(&actTitleDes);
    menu.addAction(&actTimeAs);
    menu.addAction(&actTimeDes);
    menu.addAction(&actPathAs);
    menu.addAction(&actPathDes);

    QPoint point = mapToGlobal(QPoint(_buttonSort->x(), _buttonSort->y() -menu.fontMetrics().height()-8));
    menu.exec(point, &actPathDes);
}

void PlaylistDialog::view_doubleClicked(const QModelIndex& index)
{
    _model->setCurrentTrackIndex(index);
    emit playStopCurrentTrack();
}

void PlaylistDialog::setModel(PlaylistModel* model)
{
    if( model == NULL ) return;

    if( _model != NULL ) {
        disconnect(_model, SIGNAL(fluctuatedIndexDigit()), _view, SLOT(adjustColumnSize()));
        disconnect(_buttonLoop, SIGNAL(toggled(bool)), _model, SLOT(setLoopPlay(bool)));
    }

    _model = model;

    QItemSelectionModel* m = _view->selectionModel();
    _view->setModel(_model);
    delete m;

    connect(_model, SIGNAL(fluctuatedIndexDigit()), _view, SLOT(adjustColumnSize()));
    connect(_buttonLoop, SIGNAL(toggled(bool)), _model, SLOT(setLoopPlay(bool)));
}

