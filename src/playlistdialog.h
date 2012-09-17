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

#include <QTreeWidget>
#include "ui_playlistdialog.h"

class PlayList;

class PlayListTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    PlayListTreeWidget(QWidget* parent);
    void moveItem(int from, int to) { if(from != to) insertTopLevelItem(to, takeTopLevelItem(from)); }
    void moveItems(int insert, QList<QTreeWidgetItem*>& selectedItems);

signals:
    void movedItems(int insert, QList<QTreeWidgetItem*>& items);

protected:
//  bool event(QEvent*);
    void showEvent(QShowEvent*);
    void resizeEvent(QResizeEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};

class PlayListDialog : public QDialog, Ui::PlayListDialog
{
    Q_OBJECT

public:
    PlayListDialog(PlayList* playList, QWidget* parent);

    int  selectedIndex() { return _treeWidget->indexOfTopLevelItem(_treeWidget->currentItem()); }
    void reflectDataToGui();
    void updateCurrentItemTime();
    void updateCurrentItemBackground();

signals:
    void playItem();
    void stopItem();

protected slots:
    void buttonAddClicked();
    void buttonRemoveClicked();
    void treewidgetItemDoubleClicked(QTreeWidgetItem* item, int column);
    void playListTreeWidgetMovedItems(int insert, QList<QTreeWidgetItem*>& items);

protected:
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);

private:
    PlayListTreeWidget* _treeWidget;
    QTreeWidgetItem*    _currentItem;

    PlayList* _playList;
};

#endif // PLAYLISTDIALOG_H

