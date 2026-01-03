/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/
#ifndef PAGEPRESET_MANAGER_H
#define PAGEPRESET_MANAGER_H

#include <QDomDocument>
#include <QMap>
#include <QObject>
#include <QSize>
#include <QStringList>
#include <QXmlStreamReader>
#include "margins.h"
#include "ui/marginpresetlayout.h"

using LocalizedStringsMap = QMap<QString, QString>;

enum PageSizeType
{
	Undefined = 0,
	Preset,
	User
};

struct PageCollectionInfo
{
	QString id;
	QString displayName {"undefined"};
	QString name {"undefined"}; // original name
	QString author;
	QString license;
	QString filePath;
	LocalizedStringsMap localizedNames;
	PageSizeType type {PageSizeType::Undefined};
};

struct PageSizeInfo
{
	int pageUnitIndex {-1};
	int layout {0};
	int firstPage {1};
	double width {0.0};
	double height {0.0};
	QString displayName {"undefined"};
	QString name {"undefined"}; // original name
	QString id;
	QString label;
	QString categoryId;
	QList<QString> lagacyNames; // fallback names for Scribus < 1.7.2
	QList<double> textFrame;
	LocalizedStringsMap localizedNames;
	int marginPreset {PresetLayout::none};
	MarginStruct margins;
	MarginStruct bleeds;
	PageSizeType type {PageSizeType::Undefined};
};

using PageCollectionInfoMap = QMap<QString, PageCollectionInfo>;
using PageSizeInfoMap = QMap<QString, PageSizeInfo>;

class PagePresetManager : public QObject
{
	Q_OBJECT
public:

	PagePresetManager(PagePresetManager const &) = delete;
	void operator=(PagePresetManager const &) = delete;
	static PagePresetManager &instance();

	static QStringList defaultSizesList();

	QList<QString> categoriesOrder() const { return m_categoryOrderList; }

	PageCollectionInfoMap categories() const { return m_categoryList; }

	PageCollectionInfo categoryInfoById(const QString &id);
	PageCollectionInfo categoryInfoCustom();
	PageCollectionInfo categoryInfoPreferred();

	const PageSizeInfoMap &pageSizes() const { return m_pageSizeList; }

	PageSizeInfoMap activePageSizes() const;
	PageSizeInfoMap sizesByCategory(QString categoryId) const;
	PageSizeInfoMap sizesByDimensions(QSizeF sizePt) const;

	PageSizeInfo pageInfoByDimensions(double width, double height) const { return pageInfoByDimensions(QSizeF(width, height)); }
	PageSizeInfo pageInfoByDimensions(QSizeF sizePt) const;
	PageSizeInfo pageInfoByName(const QString &name) const;
	PageSizeInfo pageInfoCustom() const;

	void reloadAllPresets();

	// Custom preset functions
	static bool createOrUpdateCollection(const QString &filePath, const PageCollectionInfo &meta, QString &outUuid);
	static bool removeCollection(const QString &filePath);
	static bool isCollectionsEmpty(const QString &filePath);
	static bool addCollectionPage(const QString &filePath, const PageSizeInfo &pageInfo);
	static bool updateCollectionPage(const QString &filePath, const PageSizeInfo &pageInfo);
	static bool removeCollectionPage(const QString &filePath, const QString &compositeId);

private:
	enum class IndexAction
	{
		Add,
		Remove
	};

	PagePresetManager(QObject *parent = nullptr);
	~PagePresetManager() = default;

	static PagePresetManager *m_instance;

	PageSizeInfoMap m_pageSizeList;
	PageCollectionInfoMap m_categoryList;
	QList<QString> m_categoryOrderList;

	LocalizedStringsMap parseNamesBlock(QXmlStreamReader &xml);
	const QString systemIndexFile();
	const QString userIndexFile();
	bool hasValidVersion(const QString &version);

	void addCategory(const PageCollectionInfo &collection);
	void addPageSize(const PageSizeInfo &info);
	void loadAllPresets(const QString &indexFilePath, PageSizeType type);
	void parseCollectionFile(const QString &filePath, PageSizeType type);

	template<typename T>
	void updateDisplayName(T &info);

	// Custom preset functions
	static bool isSizeMatch(const PageSizeInfo &info, const QSizeF &size);
	static bool updateIndexFile(const QString &collectionFilePath, IndexAction action);
	static bool loadDocument(const QString &filePath, QDomDocument &doc);
	static bool saveDocument(const QString &filePath, const QDomDocument &doc);
	static void setChildText(QDomElement &parent, const QString &tagName, const QString &text);

private slots:
	void localeChange();
};

#endif // PAGEPRESET_MANAGER_H
