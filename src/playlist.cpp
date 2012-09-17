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
#include <QStringList>
#include "playlist.h"

PlayList::Item::Item(const QString& path, const QString& title, const int timeSec)
{
    _path  = path;
    _title = title;
    setTime(timeSec);
}

void PlayList::Item::setTime(int sec)
{
    if( sec < 0 ) {
        _time.clear();
        return;
    }

    int h, m, s;
    h = sec / 3600;
    sec %= 3600;
    m = sec / 60;
    s = sec % 60;

    if( h == 0 )
        _time.sprintf("%02d:%02d", m, s);
    else
        _time.sprintf("%02d:%02d:%02d", h, m, s);
}

// ---------------------------------------------------------------------------------------
PlayList::PlayList()
{
    _currentItem = NULL;
    _loopPlay    = false;
    _randomPlay  = false;

    qsrand(time(NULL));
}

void PlayList::add(const QString& path, const int index)
{
    // カレントアイテムと同じアイテムが追加されたか判定
    bool sameCurrentItemAdded = false;
    if( _currentItem!=NULL && _currentItem->_path==path )
        sameCurrentItemAdded = true;

    QString title;
    if( path.startsWith("http://") )
        title = path;
    else
        title = path.split("/").last();

    Item item(path, title, -1);

    _items.removeOne(item); // 同一pathのアイテムは削除してから、追加し直す

    if( index < 0 )
        _items.append(item);
    else
        _items.insert(index, item);

    // カレントインデックスの設定
    if( sameCurrentItemAdded )
        _currentItem = &_items.last(); // setCurrentIndex(itemCount()-1);
    else
    if( itemCount() == 1 )  // 1個目の追加の場合
        setCurrentIndex(0);
}

void PlayList::remove(int index)
{
    if( index<0 || index>=itemCount() ) return;

    if( index == currentIndex() ) {
        _items.removeAt(index);
        setCurrentIndex(index);
    }
    else
        _items.removeAt(index);
}

PlayList::Item* PlayList::item(int index) // 範囲外指定でも訂正して値を返却している。要仕様変更
{
    if( itemCount() > 0 ) {
        if( index < 0 )
            index = 0;
        else if( index >= itemCount() )
            index = itemCount() - 1;
    
        return &_items[index];
    }
    else
        return NULL;
}

void PlayList::downCurrentIndex()
{
    int index = currentIndex() - 1;

    if( _loopPlay ) {
        if( index < 0 )
            index = itemCount() - 1;
    }

    _currentItem = item(index);
}

void PlayList::upCurrentIndex()
{
    int index = currentIndex() + 1;

    if( _loopPlay ) {
        if( index >= itemCount() )
            index = 0;
    }

    _currentItem = item(index);
}

