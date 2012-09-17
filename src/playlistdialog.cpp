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
#include <QFileDialog>
#include <QUrl>
#include <QDropEvent>
#include "playlistdialog.h"
#include "playlist.h"
#include "logdialog.h"

PlayListTreeWidget::PlayListTreeWidget(QWidget* parent) : QTreeWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus);
    setTabKeyNavigation(false);
    setProperty("showDropIndicator", QVariant(true));
    setDragEnabled(false);
    setDragDropOverwriteMode(false);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::IgnoreAction);
    setAlternatingRowColors(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setRootIsDecorated(false);
    setUniformRowHeights(false);
    setSortingEnabled(false);
    setAllColumnsShowFocus(true);
    setWordWrap(false);
    setHeaderHidden(false);
    header()->setCascadingSectionResizes(false);
 //   header()->setDefaultSectionSize(130);
    header()->setHighlightSections(false);
    header()->setProperty("showSortIndicator", QVariant(false));
    headerItem()->setText(0, tr("タイトル"));
    headerItem()->setText(1, tr("時間"));
    headerItem()->setText(2, tr("場所"));

//  header()->setClickable(true);
    header()->setSectionHidden(2, true);

    QPalette p = palette();
    p.setColor(QPalette::Base, QColor( 32,  31,  31));
    p.setColor(QPalette::Text, QColor(212, 210, 207));
    setPalette(p);
}

void PlayListTreeWidget::moveItems(int insert, QList<QTreeWidgetItem*>& items)
{
    int to = insert - 1;
    int from;

    for(int i=0; i < items.size(); i++) {
        from = indexOfTopLevelItem(items[i]);
        if( from > to )
            to++;

        moveItem(from, to);
//      LogDialog::debug(tr("%1 %2").arg(from).arg(to));
    }
}

/*
bool PlayListTreeWidget::event(QEvent* e)
{
    LogDialog::debug(tr("e %1").arg(e->type()));

    return QTreeWidget::event(e);
}
*/

void PlayListTreeWidget::showEvent(QShowEvent*)
{
    setColumnWidth(0, width()-70);
    resizeColumnToContents(1);
}

void PlayListTreeWidget::resizeEvent(QResizeEvent*)
{
    setColumnWidth(0, width()-70);
//  resizeColumnToContents(1);
}

void PlayListTreeWidget::dragEnterEvent(QDragEnterEvent* e)
{
//  QList<QTreeWidgetItem*> items = selectedItems();

//  for(int i=0; i < items.size(); i++)
//      LogDialog::debug(tr("%1").arg(items[i]->text(0)));

    QTreeWidget::dragEnterEvent(e);
}

void PlayListTreeWidget::dropEvent(QDropEvent* e)
{
    int insert;

    if( dropIndicatorPosition() == QAbstractItemView::AboveItem )
        insert = indexOfTopLevelItem(itemAt(e->pos()));
    else
    if( dropIndicatorPosition() == QAbstractItemView::BelowItem )
        insert = indexOfTopLevelItem(itemAt(e->pos())) + 1;
    else
    if( dropIndicatorPosition() == QAbstractItemView::OnViewport )
        insert = topLevelItemCount();

    QList<QTreeWidgetItem*> items = selectedItems();
    emit movedItems(insert, items);
    moveItems(insert, items);

    // 選択アイテム出力
//  for(int i=0; i < items.size(); i++)
//      LogDialog::debug(tr("s:%1").arg(items[i]->text(0)));

    // 現在のアイテム出力
//  for(int i=0; i < topLevelItemCount(); i++)
//      LogDialog::debug(tr(":%1").arg(topLevelItem(i)->text(0)));

    //QTreeWidget::dropEvent(e);
}

// ----------------------------------------------------------------------------------------
PlayListDialog::PlayListDialog(PlayList* playList, QWidget* parent) : QDialog(parent)
{
    setupUi(this);
    setAcceptDrops(true);

    _treeWidget = new PlayListTreeWidget(this);
    _verticalLayout->insertWidget(0, _treeWidget);

    _playList = playList;
    _currentItem = NULL;

    connect(_buttonAdd, SIGNAL(clicked(bool)), this, SLOT(buttonAddClicked()));
    connect(_buttonRemove, SIGNAL(clicked(bool)), this, SLOT(buttonRemoveClicked()));
    connect(_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
            this, SLOT(treewidgetItemDoubleClicked(QTreeWidgetItem*, int)));
    connect(_treeWidget, SIGNAL(movedItems(int, QList<QTreeWidgetItem*>&)),
            this, SLOT(playListTreeWidgetMovedItems(int, QList<QTreeWidgetItem*>&)));
}

void PlayListDialog::reflectDataToGui()
{
    _treeWidget->clear();

    for(int i=0; i < _playList->itemCount(); i++) {
        PlayList::Item* item = _playList->item(i);
        QStringList sl;
        sl << item->_title
           << item->_time 
           << item->_path;

        QTreeWidgetItem* twItem = new QTreeWidgetItem(sl);
        twItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
        _treeWidget->addTopLevelItem(twItem);

//      LogDialog::debug(QString().sprintf("%p %p", twItem, _treeWidget->topLevelItem(_treeWidget->topLevelItemCount()-1)));
    }

    _currentItem = NULL;
    updateCurrentItemBackground();
}

void PlayListDialog::updateCurrentItemTime()
{
    int current = _playList->currentIndex();
    if( current != -1 ) {
        PlayList::Item* item = _playList->currentItem();
        _treeWidget->topLevelItem(current)->setText(1, item->_time);
    }
}

void PlayListDialog::updateCurrentItemBackground()
{
    // 前のカレントアイテムの背景色を通常に戻す
    if( _currentItem != NULL ) {
        QBrush base = _treeWidget->palette().base();
        QBrush text = _treeWidget->palette().text();
        for(int i=0; i < _treeWidget->columnCount(); i++) {
            _currentItem->setBackground(i, base);
            _currentItem->setForeground(i, text);
        }
    }

    _currentItem = _treeWidget->topLevelItem(_playList->currentIndex());

    // カレントアイテムの背景色を変える
    if( _currentItem != NULL ) {
        QBrush base(QColor(0,0,0));
        QBrush text(QColor(106,129,198)); //QColor(255,154,231));
        for(int i=0; i < _treeWidget->columnCount(); i++) {
            _currentItem->setBackground(i, base);
            _currentItem->setForeground(i, text);
        }
    }
}

void PlayListDialog::buttonAddClicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("ファイルを選択"), "", tr("*"));

    if( !file.isEmpty() ) {
         _playList->add(file);
         reflectDataToGui();
    }

    LogDialog::debug(tr("%1").arg(_treeWidget->topLevelItemCount()));
}

void PlayListDialog::buttonRemoveClicked()
{
    int index = selectedIndex();
    if( index != -1 ) {
        int currentIndex = _playList->currentIndex();
        _playList->remove(index);
        reflectDataToGui();

        // 新たにアイテムを選択
        if( index >= _playList->itemCount() ) // 元の_playList->itemCount()-1(最大インデックス値)
            _treeWidget->setCurrentItem(_treeWidget->topLevelItem(index-1)); // NULL入力の場合あり
        else
            _treeWidget->setCurrentItem(_treeWidget->topLevelItem(index));

        if( index == currentIndex )
            emit stopItem();
    }

    LogDialog::debug(tr("PlayListDialog::buttonRemoveClicked(): %1").arg(index));
}
/*
void PlayListDialog::buttonPreviousClicked()
{
    int index = _playList->currentIndex();

    _playList->downCurrentIndex();
    if( index != _playList->currentIndex() ) {
        updateCurrentItemBackground();
        emit playItem();
    }
}

void PlayListDialog::buttonNextClicked()
{
    int index = _playList->currentIndex();

    _playList->upCurrentIndex();
    if( index != _playList->currentIndex() ) {
        updateCurrentItemBackground();
        emit playItem();
    }
}
*/

void PlayListDialog::treewidgetItemDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    int index = _treeWidget->indexOfTopLevelItem(item);
    _playList->setCurrentIndex(index);
    updateCurrentItemBackground();

    LogDialog::debug(QString("PlayListDialog::treewidgetItemDoubleClicked(): index %1")
                                                                            .arg(index));
    emit playItem();
}

void PlayListDialog::playListTreeWidgetMovedItems(int insert,
                                                  QList<QTreeWidgetItem*>& items)
{
    int to = insert - 1;
    int from;

    for(int i=0; i < items.size(); i++) {
        PlayList::Item item(items[i]->text(2), "", 0); // itemの比較はpathのみの実装の為、
                                                       // 2番目以降引数は不要
        from = _playList->indexOfItem(item);
        if( from > to )
            to++;

        _playList->move(from, to);
        LogDialog::debug(tr("%1 %2").arg(from).arg(to));
    }

    for(int i=0; i < _playList->itemCount(); i++)
        LogDialog::debug(tr("%1").arg(_playList->item(i)->_title));
}

void PlayListDialog::dragEnterEvent(QDragEnterEvent* e)
{
    if( e->mimeData()->hasFormat("text/uri-list") )
        e->acceptProposedAction();
}

void PlayListDialog::dropEvent(QDropEvent* e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if( urls.isEmpty() )
        return;

    for(int i=0; i < urls.size(); i++) {
        QString path = urls[i].toLocalFile();
        _playList->add(path);
        LogDialog::debug("PlayListDialog::dropEvent(): " + path);
    }

    reflectDataToGui();
}

