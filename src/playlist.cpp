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
#include <QMimeData>
#include <QFile>
#include <QUrl>
#include <QColor>
#include <QKeyEvent>
#include "playlist.h"
#include "commonlib.h"

PlaylistModel::Track::Track(const QString& path, const QString& title, int duration)
{
    this->path  = path;
    this->title = title;
    setTime(duration);
}

PlaylistModel::Track::Track(const Track& track)
{
    this->path     = track.path;
    this->title    = track.title;
    this->time     = track.time;
    this->duration = track.duration;
}

void PlaylistModel::Track::setTime(int duration)
{
    this->duration = duration;
    this->time     = CommonLib::secondTimeToString(duration);
}

// --------------------------------------------------------------------------------------
PlaylistModel::PlaylistModel(QObject* parent) : QAbstractTableModel(parent)
{
    setSupportedDragActions(Qt::MoveAction);

    _currentTrack = NULL;
    _loopPlay     = false;
    _randomPlay   = false;
}

PlaylistModel::~PlaylistModel()
{
    qDeleteAll(_tracks);
}

int PlaylistModel::rowCount(const QModelIndex& ) const
{
    return _tracks.size();
}

int PlaylistModel::columnCount(const QModelIndex& ) const
{
    return 2;
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const
{
    // Qt::itemDataRole role = 6,7,9,10,1,0,8
    if( !index.isValid() ) return QVariant();

//  static uint c = 0; qDebug(QString("data(): %1 %2 %3 %4").arg(index.row()).arg(index.column()).arg(role).arg(c++).toUtf8().data());

    switch( role ) {
    case Qt::TextAlignmentRole:
        if( index.column() == 1 )
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
        else
            return (int)(Qt::AlignLeft | Qt::AlignVCenter);

        break;
    case Qt::DisplayRole:
        if( index.column() == 0 )
            return _tracks[index.row()]->title;
        else
        if( index.column() == 1 )
            return _tracks[index.row()]->time;

        break;
    case Qt::BackgroundRole:
        if( _tracks[index.row()] == _currentTrack )
            return QColor(0,0,0);

        break;
    case Qt::ForegroundRole:
        if( _tracks[index.row()] == _currentTrack )
            return QColor(106,129,198);
//          return QColor(255,154,231);

        break;
    }

    return QVariant();
}
/*
bool PlaylistModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if( !index.isValid() || role != Qt::EditRole )
        return false;

    if( index.row() > _tracks.size() )
        return false;

    if( index.column() == 0 )
        _tracks[index.row()].title = value.toString();
    else
    if( index.column() == 1 )
        _tracks[index.row()].duration = value.toInt();
    else
        return false;

    emit dataChanged(index, index);
    return true;
}
*/
QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole ) {
        if( orientation == Qt::Horizontal ) {
            if( section == 0 )
                return "タイトル";
            else
                return "時間";
        }
        else
            return QString::number(section + 1);
    }

    return QVariant();
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex& index) const
{
//  static uint c = 0; if( !index.isValid() ) qDebug(QString("flags(): %1 %2").arg(index.isValid()).arg(c++).toUtf8().data());

    if( index.isValid() )
        return QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsDropEnabled;

//      return QAbstractTableModel::flags(index);
}

Qt::DropActions PlaylistModel::supportedDropActions() const
{
    return Qt::MoveAction;// | Qt::CopyAction;
}

QStringList PlaylistModel::mimeTypes() const
{
//  qDebug("mimeTypes(): ");
    return QStringList() << "application/vnd.pureplayer-tracklist";
//                       << "text/uri-list";

//  QStringList types = QAbstractTableModel::mimeTypes();
//  foreach(QString type, types)
//      qDebug(QString("mimeTypes(): %1").arg(type).toUtf8().data());

//  return QAbstractTableModel::mimeTypes();
}

QMimeData* PlaylistModel::mimeData(const QModelIndexList &indexes) const
{
    //qDebug("PlaylistModel::mimeData(): ");

    QList<int> rowList;
    foreach(const QModelIndex& index, indexes) {
        if( index.isValid() && index.column() == 0 )
            rowList << index.row();
    }

    qSort(rowList.begin(), rowList.end()); // できればdropMimeData()で行いたい

    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach(const int row, rowList) {
        const Track* p = _tracks[row];
        stream.writeRawData((char*)&p, sizeof(p)); // ポインタを格納してる為、自身用のみ
//      qDebug("data: row %d p %p", row, p);
    }

    QMimeData* mimeData = new QMimeData(); // 開放処理必要なし
    mimeData->setData("application/vnd.pureplayer-tracklist", encodedData);
    return mimeData;
}

bool PlaylistModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                 int row, int /*column*/, const QModelIndex& )
{
//  qDebug("dropMimeData(): action %04x row_column %d, %d", action, row, column);
    if( action == Qt::IgnoreAction )
        return true;

    if( row == -1 ) // アイテム上以外でドロップされた
        row = _tracks.size(); //rowCount(QModelIndex());

//  qDebug("dropMimeData(): row %d", row);

    if( data->hasFormat("application/vnd.pureplayer-tracklist") ) {
        QByteArray encodedData = data->data("application/vnd.pureplayer-tracklist");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QList<Track*> droppedTracks;

        while( !stream.atEnd() ) {
            Track* p;
            stream.readRawData((char*)&p, sizeof(p));
            droppedTracks << p;
//          qDebug("drop: p %p", p);
        }

        beginInsertRows(QModelIndex(), row, row + droppedTracks.size() - 1);

        foreach(const Track* track, droppedTracks) {
            Track* t = new Track(*track);
            if( _currentTrack == track )
                _currentTrack = t;

            _tracks.insert(row++, t);
        }

        endInsertRows();

//      foreach(const Track* t, _tracks)
//          qDebug("%s %p", t->title.toAscii().data(), t);

        return true;
/*
        // move処理
        qSort(rowList.begin(), rowList.end());
        int from, to;

        to = row - 1;
        for(int i=0; i < rowList.size(); i++) {
            from = rowList[i];
            if( from > to )
                to++;
            else
                from -= i;

            _tracks.move(from, to);
        }
*/
    }
//  else
//  if( data->hasFormat("text/uri-list") ) {
//      QList<QUrl> urls = data->urls();
//      insertTracks(row, urls);
//      return true;
//  }

    return false;
}

QModelIndex PlaylistModel::index(int row, int column, const QModelIndex& ) const
{
    if( row < 0 || row >= _tracks.size() ) return QModelIndex();

    return createIndex(row, column, _tracks[row]);
}
/*
bool PlaylistModel::insertRows(int row, int count, const QModelIndex& )
{
    if( row < 0 || row > _tracks.size() ) return false;

    for(int i=0; i < count; i++)
        _tracks.insert(row, Track());

    return true;
}
*/
bool PlaylistModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if( parent.isValid() ) return false;
    if( row < 0 || row >= _tracks.size() || count < 0 ) return false;
    if( count == 0 ) return true;

    beginRemoveRows(parent, row, row + count-1);

    for(int i=0; i < count; i++) {
        if( _tracks[row] == _currentTrack )
            _currentTrack = NULL;

        delete _tracks[row];
        _tracks.removeAt(row);
    }

    endRemoveRows();

    if( _currentTrack == NULL ) {
        if( _tracks.size() >= row+1 )
            _currentTrack = _tracks[row];
        else {
            if( row-1 >= 0 )
                _currentTrack = _tracks[row-1];
        }
    }

    return true;
}

void PlaylistModel::removeRows(QModelIndexList& indexes)
{
    qSort(indexes.begin(), indexes.end(), qGreater<QModelIndex>());

    foreach(const QModelIndex& index, indexes) {
        if( index.isValid() )
            removeRows(index.row(), 1);
    }
}

void PlaylistModel::setTracks(const QList<Track*>& tracks)
{
    qDeleteAll(_tracks);
    _tracks = tracks;
    setCurrentTrackIndex(0);
    reset();
}

bool PlaylistModel::insertTracks(int row, const QList<QUrl>& urls)
{
    QList<Track*> tracks;

    QString path, title;
    foreach(const QUrl& url, urls) {
        if( url.isLocalFile() )
            path = url.toLocalFile();
        else
            path = url.toString();

//      path.remove(QRegExp("^\\s*"));
//      if( path.isEmpty() )
//          continue;

        if( QFile::exists(path) )
            title = path.split("/").last();
        else
            title = path;

        tracks << new Track(path, title);
    }

    if( !insertTracks(row, tracks) ) {
        qDeleteAll(tracks);
        return false;
    }

    return true;
}

bool PlaylistModel::appendTrack(QString path)
{
    path.remove(QRegExp("^\\s*"));
    if( path.isEmpty() ) return false;

    QString title;
    if( QFile::exists(path) )
        title = path.split("/").last();
    else
        title = path;

    QList<Track*> tracks;
    tracks << new Track(path, title);

    if( !insertTracks(_tracks.size(), tracks) ) {
        qDeleteAll(tracks);
        return false;
    }

    return true;
}

void PlaylistModel::setCurrentTrackIndex(int index)
{
    if( 0 <= index && index < _tracks.size() ) {
        _currentTrack = _tracks[index];
        emit dataChanged(PlaylistModel::index(index, 0), PlaylistModel::index(index, 1));
    }
}

int PlaylistModel::trackIndexOf(const QString& path)
{
    for(int i=0; i < _tracks.size(); i++) {
        if( _tracks[i]->path == path )
            return i;
    }

    return -1;
}

void PlaylistModel::downCurrentTrackIndex()
{
    int i = _tracks.indexOf(_currentTrack);
    if( i == -1 ) return;

    i--;
    if( i < 0 && _loopPlay )
        i = _tracks.size() - 1;

    setCurrentTrackIndex(i);
}

void PlaylistModel::upCurrentTrackIndex()
{
    int i = _tracks.indexOf(_currentTrack);
    if( i == -1 ) return;

    i++;
    if( i >= _tracks.size() && _loopPlay )
        i = 0;

    setCurrentTrackIndex(i);
}

QString PlaylistModel::currentTrackPath()
{
    if( _currentTrack != NULL )
        return _currentTrack->path;

    return QString();
}
/*
QString PlaylistModel::trackPath(int row)
{
    if( 0 <= row && row < _tracks.size() )
        return _tracks[row]->path;

    return QString();
}
*/
void PlaylistModel::setCurrentTrackTime(int duration)
{
    if( _currentTrack != NULL )
        _currentTrack->setTime(duration);
}
/*
void PlaylistModel::test()
{
    QList<Track*> tracks;
    tracks
    << new Track("a0", "a0")
    << new Track("a1", "a1")
    << new Track("a2", "a2")
    << new Track("a3", "a3")
    << new Track("a4", "a4");

    setTracks(tracks);
    foreach(const Track* track, _tracks)
        qDebug("%p %s", track, track->path.toAscii().data());

    tracks.clear();

    tracks
    << new Track("a1", "a1")
    << new Track("", "")
    << NULL
    << new Track("  　", "empty")
    << new Track("  　/", "/")
    << new Track("a5", "a5");

    qDebug("size: %d", tracks.size());
    bool b = insertTracks(3, tracks);
    qDebug("size: %d result: %d", tracks.size(), b);
    foreach(const Track* track, _tracks)
        qDebug("%p %s", track, track->path.toAscii().data());
}
*/
bool PlaylistModel::insertTracks(int row, QList<Track*>& inTracks)
{
    if( row < 0 || row > _tracks.size() )// || inTracks.isEmpty() )
        return false;

    for(int i=0; i < inTracks.size(); i++ ) {
//      // pathの前方の空白を削除
//      if( inTracks[i] != NULL )
//          inTracks[i]->path.remove(QRegExp("^\\s*"));

        // 入力TrackがNULLまたはpathが空の場合、inTracksから削除
        if( inTracks[i]==NULL || inTracks[i]->path.isEmpty() ) {
            delete inTracks[i];
            inTracks.removeAt(i);
            i--;
            continue;
        }

        // 同一pathのTrackは_tracksから削除
        int index = trackIndexOf(inTracks[i]->path);
        if( index != -1 ) {
            if( _currentTrack == _tracks[index] )
                _currentTrack = inTracks[i];

            beginRemoveRows(QModelIndex(), index, index);
            delete _tracks[index];
            _tracks.removeAt(index);
            endRemoveRows();
            if( index < row )
                row--;
        }
    }

    if( inTracks.size() == 0 )
        return false;

    // 初めからトラックが1件もない場合、カレントを先頭に設定する
    if( _currentTrack == NULL )
        _currentTrack = inTracks[0];

    beginInsertRows(QModelIndex(), row, row + inTracks.size()-1);

    foreach(Track* track, inTracks)
        _tracks.insert(row++, track);

    endInsertRows();

    return true;
}

// --------------------------------------------------------------------------------------
PlaylistView::PlaylistView(QWidget* parent) : QTreeView(parent)
{
    setFocusPolicy(Qt::ClickFocus);
    setTabKeyNavigation(false);

    setAcceptDrops(true);
    setDragEnabled(true);
    setDropIndicatorShown(true);
    setDragDropOverwriteMode(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // PlaylistModelが返すmimeデータが自身のポインタなので外部ドロップは受け付けない
    setDragDropMode(QAbstractItemView::InternalMove);
//  setDragDropMode(QAbstractItemView::DragDrop);

    setDefaultDropAction(Qt::IgnoreAction);
    setRootIsDecorated(false);
    setAllColumnsShowFocus(true);
//  setExpandsOnDoubleClick(false);
    setItemsExpandable(false);

    setSortingEnabled(false);
    setWordWrap(false);

    QPalette p = palette();
    p.setColor(QPalette::Base, QColor( 32,  31,  31));
    p.setColor(QPalette::Text, QColor(212, 210, 207));
    setPalette(p);
}

void PlaylistView::showEvent(QShowEvent* )
{
    setColumnWidth(0, width()-70);
    resizeColumnToContents(1);
}

void PlaylistView::resizeEvent(QResizeEvent* )
{
    setColumnWidth(0, width()-70);
}

void PlaylistView::keyPressEvent(QKeyEvent* e)
{
    switch( e->key() ) {
    case Qt::Key_Return:
        emit pressedReturnKey(currentIndex());
        break;
    default:
        QTreeView::keyPressEvent(e);
    }
}

void PlaylistView::dragEnterEvent(QDragEnterEvent* e)
{
//  qDebug("PlaylistView::dragEnterEvent():");
    if( e->mimeData()->hasFormat("text/uri-list") )
        e->accept();
    else
        QTreeView::dragEnterEvent(e);
}

void PlaylistView::dropEvent(QDropEvent* e)
{
//  foreach(QString type, e->mimeData()->formats())
//      qDebug("PlaylistView::dropEvent(): %s", type.toAscii().data());

    if( e->mimeData()->hasFormat("text/uri-list") ) {
        PlaylistModel* m = (PlaylistModel*)model();
        if( m != NULL )
            m->insertTracks(m->rowCount(), e->mimeData()->urls());
    }
    else
        QTreeView::dropEvent(e);
}

