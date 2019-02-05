/****************************************************************************
 * Copyright (C) 2016,2017 Maschell
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
 ****************************************************************************/
#ifndef _CursorDrawer_H_
#define _CursorDrawer_H_

#include <string>

#include <stdio.h>
#include <stdint.h>

class SimpleColorDrawer {

public:
    static SimpleColorDrawer *getInstance() {
        if(!instance)
            instance = new SimpleColorDrawer();
        return instance;
    }

    static void destroyInstance() {
        if(instance) {
            delete instance;
            instance = NULL;
        }
    }

    static void draw(float x, float y, float width, float height) {
        SimpleColorDrawer * cur_instance = getInstance();
        if(cur_instance ==  NULL) return;
        cur_instance->draw_internal(x,y,width,height);
    }

private:
    //!Constructor
    SimpleColorDrawer();
    //!Destructor
    ~SimpleColorDrawer();
    static SimpleColorDrawer *instance;
    void draw_internal(float x, float y, float width, float height);
    void init_colorVtxs();

    uint8_t * colorVtxs = NULL;
};

#endif
