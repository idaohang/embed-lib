/*
 * Filename: screen.cpp
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Implementation file for Screen singleton class which wraps ncurses
 */

#include "screen.h"

Screen* Screen::m_instance = NULL;

Screen* Screen::Instance()
{
    if (!m_instance)
        m_instance = new Screen;

    return m_instance;
}

bool Screen::init ()
{
    // Initialize screen
    initscr();

    // Do not block on getch
    if (nodelay(stdscr, true) != OK)
    {
        destroy();
        fprintf(stderr, "Screen::init nodelay error\n");
        return false;
    }

    // Do not output keyboard input to screen
    if (noecho() != OK)
    {
        destroy();
        fprintf(stderr, "Screen::init noecho error\n");
        return false;
    }

    return true;
}

void Screen::destroy ()
{
    endwin();
}

void Screen::size (int32_t& _width, int32_t& _height)
{
    getmaxyx(stdscr, _height, _width);
}

char Screen::getCh( )
{
    return getch();
}

char Screen::getChSync()
{
    nodelay(stdscr, false);
    char c = getch();
    nodelay(stdscr, true);
    return c;
}

void Screen::printRectangle (int32_t _x1, int32_t _y1, int32_t _x2, int32_t _y2, char _char)
{
    if (_x2 < _x1)
    {
        fprintf(stderr, "Screen::printBorder error _x2 < _x1\n");
        return;
    }
    else if (_y2 < _y1)
    {
        fprintf(stderr, "Screen::printBorder error _y2 < _y1\n");
        return;
    }

    for (int32_t i = _x1; i <= _x2; i++)
    {
        move (_y1, i);
        addch (_char);
    }
    for (int32_t i = _x1; i <= _x2; i++)
    {
        move (_y2, i);
        addch (_char);
    }
    for (int32_t i = _y1; i <= _y2; i++)
    {
        move (i, _x1);
        addch (_char);
    }
    for (int32_t i = _y1; i <= _y2; i++)
    {
        move (i, _x2);
        addch (_char);
    }
    refresh();
}

void Screen::printHLine (int32_t _x1, int32_t _x2, int32_t _y, char _char)
{
    for (int32_t i = _x1; i <= _x2; i++)
    {
        move (_y, i);
        addch (_char);
    }
}

void Screen::printVLine (int32_t _x, int32_t _y1, int32_t _y2, char _char)
{
    for (int32_t i = _y1; i <= _y2; i++)
    {
        move (i, _x);
        addch (_char);
    }
}

void Screen::printText (int32_t _x, int32_t _y, const char* _text)
{
    move (_y, _x);
    printw(_text);
    refresh();
}

void Screen::printText (int32_t _x, int32_t _y, std::string _text)
{
    move (_y, _x);
    printw(_text.c_str());
    refresh();
}

void Screen::printTextCenter (int32_t _x1, int32_t _x2, int32_t _y, const char* _text)
{
    int32_t x1Text = ((_x2 - _x1) / 2) - (strlen(_text) / 2);
    move (_y, x1Text);
    printw(_text);
    refresh();
}

void Screen::printTextCenter (int32_t _x1, int32_t _x2, int32_t _y, std::string _text)
{
    int32_t x1Text = ((_x2 - _x1) / 2) - (_text.length() / 2);
    move (_y, x1Text);
    printw(_text.c_str());
    refresh();
}
