/**
 * @file    stylemanager.h
 * @author  Ali Sakkaf
 */
#pragma once
#include <QObject>
#include "apptheme.h"

class StyleManager : public QObject
{
    Q_OBJECT
public:
    static StyleManager &instance();

    AppTheme::Mode currentMode() const { return m_mode; }
    bool           isDark()      const { return m_mode == AppTheme::Dark; }

    void setMode(AppTheme::Mode m);
    void toggleTheme();

signals:
    void themeChanged(AppTheme::Mode newMode);

private:
    explicit StyleManager(QObject *parent = nullptr);
    AppTheme::Mode m_mode = AppTheme::Dark;
};
