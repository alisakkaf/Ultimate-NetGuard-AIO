/**
 * @file    apptheme.h
 * @author  Ali Sakkaf
 */
#pragma once
#include <QApplication>
#include <QString>

class AppTheme
{
public:
    enum Mode { Dark, Light };

    static void   apply(QApplication *app, Mode m);
    static void   toggle(QApplication *app, Mode &current);
    static QString darkQss();
    static QString lightQss();

private:
    AppTheme() = delete;
};
