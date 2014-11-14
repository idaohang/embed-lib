/*
 * Filename: screen.h
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Header file for Screen singleton class that wraps ncurses
 */

#ifndef EMBED_SCREEN_H
#define EMBED_SCREEN_H

#include <sys/types.h>
#include <ncurses.h>
#include <string>
#include <string.h>

class Screen
{
 public:
    // Singleton
    static Screen* Instance();

    bool init();
    void destroy();

    // Accessors
    void size(int32_t& _width, int32_t& _height);
    char getCh();
    char getChSync();

    // Modifiers
    void printRectangle (int32_t _x1, int32_t _y1, int32_t _x2, int32_t _y2, char _char = '#');
    void printHLine (int32_t _x1, int32_t _x2, int32_t _y, char _char = '#');
    void printVLine (int32_t _x, int32_t _y1, int32_t _y2, char _char = '#');
    void printText (int32_t _x, int32_t _y, const char* _text);
    void printText (int32_t _x, int32_t _y, std::string _text);
    void printTextCenter (int32_t _x1, int32_t _x2, int32_t _y, const char* _text);
    void printTextCenter (int32_t _x1, int32_t _x2, int32_t _y, std::string _text);
 private:
    // Private so they cannot be called
    Screen() {};
    Screen(Screen const&) {};
    Screen& operator=(Screen const&) {return (*m_instance);};

    // Singleton
    static Screen* m_instance;
};

#endif
