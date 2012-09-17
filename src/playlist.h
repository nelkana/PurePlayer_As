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
#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <time.h>
#include <QString>
#include <QList>

class PlayList
{
public:
    class Item
    {
    public:
        QString _path;
        QString _title;
        QString _time;

        Item(const QString& path, const QString& title, const int timeSec=-1);
        void setTime(int sec);
                                                                    //パスが同じなら等しい
        bool operator==(const Item& other) const { return _path == other._path; }
    };

    PlayList();
    void  add(const QString& path, const int index=-1);
    void  remove(int index);
    void  move(int from, int to) { _items.move(from, to);  }
    Item* item(int index);
    int   indexOfItem(Item& item) { return _items.indexOf(item); }
    int   itemCount() { return _items.size(); }

    void  setCurrentIndex(int index) { _currentItem = item(index); }
    int   currentIndex() { if(itemCount()<=0) return -1; else return _items.indexOf(*_currentItem); }
    void  downCurrentIndex();
    void  upCurrentIndex();
    Item* currentItem() { return _currentItem; } // Itemの内容は書き込み不可の方が良い

    void  setLoopPlay(bool b)    { _loopPlay = b;   }
    void  setRandomPlay(bool b)  { _randomPlay = b; }

private:
    QList<Item> _items;

    Item* _currentItem;
    bool  _loopPlay;
    bool  _randomPlay;
};

#endif //  PLAYLIST_H

