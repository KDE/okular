/*
    SPDX-FileCopyrightText: 2019 Bubli <Katarina.Behrens@cib.de>
    SPDX-FileCopyrightText: 2020 Albert Astals Cid <albert.astals.cid@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PDFSETTINGSWIDGET_H
#define PDFSETTINGSWIDGET_H

#include <QWidget>

#include "ui_pdfsettingswidget.h"
#include <poppler-form.h>
#include <poppler-version.h>

#define POPPLER_VERSION_MACRO ((POPPLER_VERSION_MAJOR << 16) | (POPPLER_VERSION_MINOR << 8) | (POPPLER_VERSION_MICRO))

class QTreeWidget;

class PDFSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PDFSettingsWidget(QWidget *parent = nullptr);
    bool event(QEvent *e) override;
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 06, 0)
    static std::optional<Poppler::CryptoSignBackend> settingStringToPopplerEnum(QStringView backend);
    static QString popplerEnumToSettingString(Poppler::CryptoSignBackend backend);
#endif

private:
    void warnRestartNeeded();

    QTreeWidget *m_tree = nullptr;
    bool m_certificatesAsked = false;
    bool m_warnedAboutRestart = false;
    Ui_PDFSettingsWidgetBase m_pdfsw;
};

#endif
