/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NEWNOTETOOL_H_
#define NEWNOTETOOL_H_

#include "EventTool.h"

class NewNoteTool : public EventTool {

    friend class WindowContext;

public:
    NewNoteTool();
    NewNoteTool(NewNoteTool& other);

    ProtocolEntry* copy();
    void reloadState(ProtocolEntry* entry);

    void draw(QPainter* painter);
    bool press(bool leftClick);
    bool release();
    bool move(int mouseX, int mouseY);
    bool releaseOnly();

    static int editTrack();
    static int editChannel();
    static void setEditTrack(int i);
    static void setEditChannel(int i);

    static int noteDurationDivisor();
    static void setNoteDurationDivisor(int divisor);

    bool pressKey(int key) override;

private:
    bool inDrag;
    int line;
    int xPos;
    static int _channel, _track;
    static int _noteDurationDivisor;  // 0 = use drag, 1 = whole, 2 = half, 3 = third, 4 = quarter, etc.
};

#endif
