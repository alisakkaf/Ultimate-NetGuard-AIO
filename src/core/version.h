/**
 * @file    version.h
 * @brief   Centralized version and publisher metadata for Ultimate NetGuard AIO.
 *          All modules should #include this file to access version constants.
 * @author  Ali Sakkaf
 */
#pragma once

// ─── Version Numbers ─────────────────────────────────────────────────────────
#define APP_VERSION_MAJOR   1
#define APP_VERSION_MINOR   0
#define APP_VERSION_PATCH   0
#define APP_VERSION_BUILD   0

// Comma-separated for VERSIONINFO resource
#define APP_VERSION_RC      APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_PATCH,APP_VERSION_BUILD

// String versions
#define APP_VERSION_STR     "1.0.0"
#define APP_VERSION_RC_STR  "1.0.0.0"

// ─── Application Identity ────────────────────────────────────────────────────
#define APP_NAME            "Ultimate NetGuard AIO"
#define APP_INTERNAL_NAME   "UltimateNetGuard"
#define APP_DESCRIPTION     "Professional Network & System Monitor - All In One"
#define APP_ORIGINAL_NAME   "UltimateNetGuard.exe"

// ─── Publisher / Author ──────────────────────────────────────────────────────
#define APP_AUTHOR          "Ali Sakkaf"
#define APP_COMPANY         "Ali Sakkaf"
#define APP_COPYRIGHT       "Copyright \\xA9 2026 Ali Sakkaf. All Rights Reserved."

// ─── Online Presence ─────────────────────────────────────────────────────────
#define APP_WEBSITE         "https://alisakkaf.com"
#define APP_GITHUB          "https://github.com/alisakkaf"
#define APP_FACEBOOK        "https://www.facebook.com/AliSakkaf.Dev/"

// ─── Display String (used in UI header versionLabel) ─────────────────────────
#define APP_VERSION_DISPLAY "V1.0.0 | Ali Sakkaf"
