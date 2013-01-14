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
#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractTableModel>
#include <QTreeView>
#include "commonlib.h"

class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    class Track
    {
    public:
        QString path;
        QString title;
        QString time;
        int     duration;
        qint16  index;      // ソート用メモリの無駄

        Track(const QString& path=QString(), const QString& title=QString(), int duration=-1);
        Track(const Track& track);

        void setTime(int duration);
                                                                    //パスが同じなら等しい
//      bool operator==(const Track& other) const { return this->path == other.path; }
    };

    enum {
        COLUMN_COUNT = 3,
        COLUMN_INDEX = 0,
        COLUMN_TITLE = 1,
        COLUMN_TIME  = 2,
        COLUMN_PATH  = 255, // 実際には存在しないカラム
    };

    explicit PlaylistModel(QObject* parent);
    virtual ~PlaylistModel();

    int         rowCount(const QModelIndex& parent=QModelIndex()) const;
    int         columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant    data(const QModelIndex& index, int role) const;
//  bool        setData(const QModelIndex& index, const QVariant& value, int role=Qt::EditRole);
    QVariant    headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags   flags(const QModelIndex& index) const;
    Qt::DropActions supportedDropActions() const;

    QStringList mimeTypes() const;
    QMimeData*  mimeData(const QModelIndexList &indexes) const;
    bool        dropMimeData(const QMimeData *data, Qt::DropAction action,
                             int row, int column, const QModelIndex &parent);
    QModelIndex index(int row, int column, const QModelIndex& parent=QModelIndex()) const;
//  bool        insertRows(int row, int count, const QModelIndex& parent=QModelIndex());
    bool        removeRows(int row, int count, const QModelIndex& parent=QModelIndex());
    void        removeRows(QModelIndexList&);
    void        sort(int column, Qt::SortOrder order=Qt::AscendingOrder);

    void    setTracks(const QList<Track*>& tracks);
    int     insertTracks(int row, const QList<QUrl>& urls);
    int     appendTracks(const QStringList& paths);
    void    setCurrentTrackIndex(int index);
    int     trackIndexOf(const QString& path);
    void    downCurrentTrackIndex();
    void    upCurrentTrackIndex();
    Track*  currentTrack() { return _currentTrack; }
    bool    isCurrentTrack(const QString& path) { return (_currentTrack!=NULL && path==_currentTrack->path); }
    QString currentTrackTitle();
    QString currentTrackPath();
    QString trackPath(int row);
    void    setCurrentTrackTitle(const QString& title);
    void    setCurrentTrackTime(int sec);
    bool    loopPlay()   { return _loopPlay; }
    bool    randomPlay() { return _randomPlay; }

public slots:
    void setCurrentTrackIndex(const QModelIndex& index) { if( index.isValid() ) setCurrentTrackIndex(index.row()); }
    void setLoopPlay(bool b)   { _loopPlay = b; }
    void setRandomPlay(bool b) { _randomPlay = b; }
    void removeAllRows();

//  void test();
signals:
    void removedCurrentTrack();
    void fluctuatedIndexDigit();

protected:
    int insertTracks(int row, QList<Track*>& tracks);
    QList<Track*> createTracks(const QStringList& paths);

private:
    QList<Track*> _tracks;

    Track* _currentTrack;
    bool   _loopPlay;
    bool   _randomPlay;

    CommonLib::EmitDeterFlag _emitFlag;
};

class PlaylistView : public QTreeView
{
    Q_OBJECT

public:
    explicit PlaylistView(QWidget* parent);

public slots:
    void adjustColumnSize();
    void removeSelectedTrack();
    void sortTitleAscending();
    void sortTitleDescending();
    void sortTimeAscending();
    void sortTimeDescending();
    void sortPathAscending();
    void sortPathDescending();

signals:
    void pressedReturnKey(const QModelIndex&);

protected slots:
    virtual void slot_customContextMenuRequested(const QPoint&);
    void actOpenMediaLocation_triggered();

protected:
    void showEvent(QShowEvent*);
    void resizeEvent(QResizeEvent*);
    void keyPressEvent(QKeyEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);

    void openMediaLocation(const QString& path);

    QMenu* _menu;
    QList<QAction*> _actionsForFile;

private:
    void createContextMenu();
};

/*
class PeercastPlaylistView : public PlaylistView
{
    Q_OBJECT

public:
    PeercastPlaylistView(QWidget* parent) : PlaylistView(parent) {}
};
*/

/*
class PlayList
{
public:
    PlayList();
//  void  add(const QString& path, const int index=-1);
    void  remove(int index);
    void  move(int from, int to) { _items.move(from, to);  }
    Item* item(int index);
    int   indexOfItem(Item& item) { return _items.indexOf(item); }
//  int   itemCount() { return _items.size(); }

//  void  setCurrentIndex(int index) { _currentItem = item(index); }
    int   currentIndex() { if(itemCount()<=0) return -1; else return _items.indexOf(*_currentItem); }
//  void  downCurrentIndex();
//  void  upCurrentIndex();
    Item* currentItem() { return _currentItem; }

//  void  setLoopPlay(bool b)    { _loopPlay = b;   }
//  void  setRandomPlay(bool b)  { _randomPlay = b; }

private:
    PlaylistModel _model;
};
*/

#endif //PLAYLISTMODEL_H

