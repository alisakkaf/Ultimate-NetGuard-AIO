/**
 * @file    stylemanager.cpp
 * @author  Ali Sakkaf
 */

#include "stylemanager.h"
#include <QApplication>

StyleManager &StyleManager::instance()
{
    static StyleManager s;
    return s;
}

StyleManager::StyleManager(QObject *parent) : QObject(parent) {}

void StyleManager::setMode(AppTheme::Mode m)
{
    m_mode = m;
    AppTheme::apply(qApp, m_mode);
    emit themeChanged(m_mode);
}

void StyleManager::toggleTheme()
{
    setMode(m_mode == AppTheme::Dark ? AppTheme::Light : AppTheme::Dark);
}
