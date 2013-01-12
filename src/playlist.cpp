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
#include <QDesktopServices>
#include <QMimeData>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QColor>
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
    return COLUMN_COUNT;
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const
{
    // Qt::itemDataRole role = 6,7,9,10,1,0,8
    if( !index.isValid() ) return QVariant();

//  static uint c = 0; qDebug(QString("data(): %1 %2 %3 %4").arg(index.row()).arg(index.column()).arg(role).arg(c++).toUtf8().data());

    switch( role ) {
    case Qt::TextAlignmentRole:
        if( index.column() == COLUMN_INDEX || index.column() == COLUMN_TIME )
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
        else
            return (int)(Qt::AlignLeft | Qt::AlignVCenter);

        break;
    case Qt::DisplayRole:
        if( index.column() == COLUMN_INDEX )
            return QString::number(index.row() + 1) + '.';
        else
        if( index.column() == COLUMN_TITLE )
            return _tracks[index.row()]->title;
        else
        if( index.column() == COLUMN_TIME )
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
        else
        if( index.column() == 0 )
            return QColor(128,128,128);

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
            if( section == COLUMN_INDEX )
                return QVariant();
            else if( section == COLUMN_TITLE )
                return "タイトル";
            else if( section == COLUMN_TIME )
                return "時間";
        }
        else
            return QString::number(section + 1);
    }

    return QAbstractTableModel::headerData(section, orientation, role);
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
        if( index.isValid() && index.column() == COLUMN_INDEX )
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

    int oldIndexDigit = CommonLib::digit(_tracks.size()); // トラック数の桁増減確認用

    // count訂正
    if( row + count-1 >= _tracks.size() )
        count = _tracks.size() - row;

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

        if( _emitFlag.toEmit() )
            emit removedCurrentTrack();
    }

    // トラック数の桁増減確認
    if( oldIndexDigit != CommonLib::digit(_tracks.size()) )
        emit fluctuatedIndexDigit();

    return true;
}

void PlaylistModel::removeRows(QModelIndexList& indexes)
{
    qSort(indexes.begin(), indexes.end(), qGreater<QModelIndex>());

    _emitFlag.begin();

    foreach(const QModelIndex& index, indexes) {
        if( index.isValid() )
            removeRows(index.row(), 1);
    }

    _emitFlag.end();

    if( _emitFlag.emitted() )
        emit removedCurrentTrack();
}

class TrackLessThan
{
public:
    TrackLessThan(int column, Qt::SortOrder order)
    { _column = column; _order = order; }

    bool operator()(PlaylistModel::Track* n1, PlaylistModel::Track* n2)
    {
        int ret = 0;

        if( _column == PlaylistModel::COLUMN_TITLE ) {
            QString str1 = n1->title.toLower(),
                    str2 = n2->title.toLower();

            ret = compare(str1, str2);
            if( ret == 0 )
                ret = compare(n1->duration, n2->duration);
        }
        else
        if( _column == PlaylistModel::COLUMN_TIME ) {
            ret = compare(n1->duration, n2->duration);
            if( ret == 0 ) {
                QString str1 = n1->title.toLower(),
                        str2 = n2->title.toLower();

                ret = compare(str1, str2);
            }
        }
        else
        if( _column == PlaylistModel::COLUMN_PATH ) {
            ret = compare(n1->path, n2->path);
        }

        if( ret != 0 ) {
            if( _order == Qt::AscendingOrder )
                return ret < 0;
            else
                return ret > 0;
        }

        return n1->index < n2->index;
    }

    template <typename T>
    int compare(T n1, T n2)
    {
        if( n1 == n2 )     return 0;
        else if( n1 < n2 ) return -1;
        else               return 1;
    }

private:
    int _column;
    Qt::SortOrder _order;
};

void PlaylistModel::sort(int column, Qt::SortOrder order)
{
    if( column == COLUMN_INDEX ) return;
    if( column < 0 ) return;
    if( _tracks.isEmpty() ) return;

    emit layoutAboutToBeChanged();

    for(int i=0; i < _tracks.size(); i++)
        _tracks[i]->index = i;

    qSort(_tracks.begin(), _tracks.end(), TrackLessThan(column, order));

    QVector<int> toIndexes(_tracks.size());
    for(int i=0; i < _tracks.size(); i++)
        toIndexes[_tracks[i]->index] = i;

    QModelIndexList from = persistentIndexList();
    QModelIndexList to;

    foreach(const QModelIndex& i, from)
        to << index(toIndexes[i.row()], i.column());

    changePersistentIndexList(from, to);

    emit layoutChanged();

//  qDebug("PlaylistModel::sort():");
//  for(int i=0; i < from.size(); i++)
//      qDebug("from: %d to: %d column: %d", from[i].row(), to[i].row(), from[i].column());
}

void PlaylistModel::setTracks(const QList<Track*>& tracks)
{
    int oldIndexDigit = CommonLib::digit(_tracks.size()); // トラック数の桁増減確認用

    qDeleteAll(_tracks);
    _tracks = tracks;
    if( _currentTrack != NULL ) {
        _currentTrack = NULL;
        emit removedCurrentTrack();
    }
    setCurrentTrackIndex(0);

    reset();

    // トラック数の桁増減確認
    if( oldIndexDigit != CommonLib::digit(_tracks.size()) )
        emit fluctuatedIndexDigit();
}

int PlaylistModel::insertTracks(int row, const QList<QUrl>& urls)
{
    QStringList paths;
    foreach(const QUrl& url, urls)
        paths << url.toString();

    QList<Track*> tracks = createTracks(paths);

    int rows = insertTracks(row, tracks);
    if( !rows )
        qDeleteAll(tracks);

    return rows;
}

int PlaylistModel::appendTracks(const QStringList& paths)
{
//  path.remove(QRegExp("^\\s*"));
//  if( path.isEmpty() ) return false;

    QList<Track*> tracks = createTracks(paths);

    int rows = insertTracks(_tracks.size(), tracks);
    if( !rows )
        qDeleteAll(tracks);

    return rows;
}

void PlaylistModel::setCurrentTrackIndex(int index)
{
    if( 0 <= index && index < _tracks.size() ) {
        _currentTrack = _tracks[index];
        emit dataChanged(PlaylistModel::index(index, 0), PlaylistModel::index(index, columnCount()-1));
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

QString PlaylistModel::currentTrackTitle()
{
    if( _currentTrack != NULL )
        return _currentTrack->title;

    return QString();
}

QString PlaylistModel::currentTrackPath()
{
    if( _currentTrack != NULL )
        return _currentTrack->path;

    return QString();
}

QString PlaylistModel::trackPath(int row)
{
    if( 0 <= row && row < _tracks.size() )
        return _tracks[row]->path;

    return QString();
}

void PlaylistModel::setCurrentTrackTitle(const QString& title)
{
    int i = _tracks.indexOf(_currentTrack);
    if( i == -1 ) return;

    _currentTrack->title = title;
    emit dataChanged(PlaylistModel::index(i, 0), PlaylistModel::index(i, columnCount()-1));
}

void PlaylistModel::setCurrentTrackTime(int duration)
{
    int i = _tracks.indexOf(_currentTrack);
    if( i == -1 ) return;

    _currentTrack->setTime(duration);
    emit dataChanged(PlaylistModel::index(i, 0), PlaylistModel::index(i, columnCount()-1));
}

void PlaylistModel::removeAllRows()
{
    qDeleteAll(_tracks);
    _tracks.clear();
    if( _currentTrack != NULL ) {
        _currentTrack = NULL;
        emit removedCurrentTrack();
//      emit fluctuatedIndexDigit();
    }

    reset();
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
int PlaylistModel::insertTracks(int row, QList<Track*>& inTracks)
{
    if( row < 0 || row > _tracks.size() )// || inTracks.isEmpty() )
        return 0;

    int oldIndexDigit = CommonLib::digit(_tracks.size()); // トラック数の桁増減確認用

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
        return 0;

    beginInsertRows(QModelIndex(), row, row + inTracks.size()-1);

    foreach(Track* track, inTracks)
        _tracks.insert(row++, track);

    endInsertRows();

    // 初めからトラックが1件もない場合
    if( _currentTrack == NULL ) {
        _currentTrack = inTracks[0]; // カレントを先頭に設定する
        emit fluctuatedIndexDigit();
    }
    else
    if( oldIndexDigit != CommonLib::digit(_tracks.size()) ) // トラック数の桁増減確認
        emit fluctuatedIndexDigit();

    return inTracks.size();
}

QList<PlaylistModel::Track*> PlaylistModel::createTracks(const QStringList& paths)
{
    QList<Track*> tracks;

    foreach(QString path, paths) {
        QUrl url(path);
        if( url.isLocalFile() )
            path = url.toLocalFile();

        QDir dir(path);
        if( dir.exists() ) {
            QStringList files = dir.entryList(QString(CommonLib::MEDIA_FORMATS).split(" "),
                                              QDir::Files);
            foreach(const QString& file, files)
                tracks << new Track(dir.absoluteFilePath(file), file);
        }
        else {
            QString title;
            if( QFile::exists(path) )
                title = path.split("/").last();
            else {
                if( path.indexOf(QRegExp("^\\s*$")) != -1 )
                    continue;

                title = path;
            }

            tracks << new Track(path, title);
        }
    }

    return tracks;
}

// --------------------------------------------------------------------------------------
#include <QKeyEvent>
#include <QScrollBar>
#include <QHeaderView>
#include <QMenu>

PlaylistView::PlaylistView(QWidget* parent) : QTreeView(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slot_customContextMenuRequested(const QPoint&)));

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

    setSortingEnabled(true);
    setWordWrap(false);

    setHeaderHidden(true);
/*  header()->setSortIndicatorShown(true);
    header()->setMovable(false);
    header()->setResizeMode(QHeaderView::Fixed);
    header()->setSortIndicator(-1, Qt::DescendingOrder);//Qt::AscendingOrder);
*/
    QPalette p = palette();
    p.setColor(QPalette::Base, QColor( 32,  31,  31));
    p.setColor(QPalette::Text, QColor(212, 210, 207));
    setPalette(p);

    QFont f = font();
    if( f.pointSize() >= 9 ) {
        f.setPointSize(8);
        setFont(f);
    }

    createContextMenu();
}

void PlaylistView::adjustColumnSize()
{
    resizeColumnToContents(0);

    int w = width() - columnWidth(0) - (fontMetrics().width("000:00:00") + 6);
    if( verticalScrollBar()->isVisible() )
        w -= verticalScrollBar()->width();

    setColumnWidth(1, w);
    resizeColumnToContents(2);
}

void PlaylistView::removeSelectedTrack()
{
    PlaylistModel* m = (PlaylistModel*)model();
    if( m != NULL ) {
        QModelIndexList indexes = selectionModel()->selectedRows();

        m->removeRows(indexes);
    }
}

void PlaylistView::sortTitleAscending()
{
    ((PlaylistModel*)model())->sort(PlaylistModel::COLUMN_TITLE, Qt::AscendingOrder);
}

void PlaylistView::sortTitleDescending()
{
    ((PlaylistModel*)model())->sort(PlaylistModel::COLUMN_TITLE, Qt::DescendingOrder);
}

void PlaylistView::sortTimeAscending()
{
    ((PlaylistModel*)model())->sort(PlaylistModel::COLUMN_TIME, Qt::AscendingOrder);
}

void PlaylistView::sortTimeDescending()
{
    ((PlaylistModel*)model())->sort(PlaylistModel::COLUMN_TIME, Qt::DescendingOrder);
}

void PlaylistView::sortPathAscending()
{
    ((PlaylistModel*)model())->sort(PlaylistModel::COLUMN_PATH, Qt::AscendingOrder);
}

void PlaylistView::sortPathDescending()
{
    ((PlaylistModel*)model())->sort(PlaylistModel::COLUMN_PATH, Qt::DescendingOrder);
}

void PlaylistView::slot_customContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = indexAt(pos);
    if( index.isValid() ) {
        foreach(QAction* action, _actionsForFile)
            action->setVisible(true);

        QString path = ((PlaylistModel*)model())->trackPath(index.row());
        QFileInfo f(path);
        if( f.isFile() || f.isDir() )
            _actionsForFile[0]->setEnabled(true);
        else
            _actionsForFile[0]->setEnabled(false);

    }
    else {
        foreach(QAction* action, _actionsForFile)
            action->setVisible(false);
    }

    _menu->exec(viewport()->mapToGlobal(pos));
}

void PlaylistView::actOpenMediaLocation_triggered()
{
    QModelIndex index = indexAt(viewport()->mapFromGlobal(_menu->pos()));
    if( index.isValid() ) {
        QString path = ((PlaylistModel*)model())->trackPath(index.row());
        openMediaLocation(path);
    }
}

void PlaylistView::showEvent(QShowEvent* e)
{
    adjustColumnSize();

    QTreeView::showEvent(e);
}

void PlaylistView::resizeEvent(QResizeEvent* e)
{
    adjustColumnSize();

    QTreeView::resizeEvent(e);
}

void PlaylistView::keyPressEvent(QKeyEvent* e)
{
    switch( e->key() ) {
    case Qt::Key_Return:
        emit pressedReturnKey(currentIndex());
        break;
    case Qt::Key_Delete:
        removeSelectedTrack();
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

void PlaylistView::openMediaLocation(const QString& path)
{
//  qDebug("%s", path.toAscii().data());
    QFileInfo f(path);
    if( f.isFile() )
        QDesktopServices::openUrl(QUrl::fromLocalFile(f.path()));
    else
    if( f.isDir() )
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void PlaylistView::createContextMenu()
{
    QMenu* sortMenu = new QMenu(tr("ソート"), this);
    sortMenu->addAction(tr("タイトル昇順"), this, SLOT(sortTitleAscending()));
    sortMenu->addAction(tr("タイトル降順"), this, SLOT(sortTitleDescending()));
    sortMenu->addAction(tr("時間昇順"), this, SLOT(sortTimeAscending()));
    sortMenu->addAction(tr("時間降順"), this, SLOT(sortTimeDescending()));
    sortMenu->addAction(tr("パス昇順"), this, SLOT(sortPathAscending()));
    sortMenu->addAction(tr("パス降順"), this, SLOT(sortPathDescending()));

    _menu = new QMenu(this);
    _actionsForFile << _menu->addAction("場所を開く", this, SLOT(actOpenMediaLocation_triggered()));
    _actionsForFile << _menu->addSeparator();
    _actionsForFile << _menu->addAction("選択項目を削除", this, SLOT(removeSelectedTrack()), tr("del"));
    _actionsForFile << _menu->addSeparator();

    _menu->addMenu(sortMenu);
}

