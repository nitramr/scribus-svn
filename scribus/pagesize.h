/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
    begin                : Feb 2005
    copyright            : (C) 2005 by Craig Bradney
    email                : cbradney@scribus.info
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#ifndef PAGESIZE_H
#define PAGESIZE_H

#include <QMap>
#include <QString>
#include <QSize>
#include "scribusapi.h"
#include "units.h"
#include "manager/pagepreset_manager.h"

class SCRIBUS_API PageSize
{
public:
	PageSize(const QString&);
	PageSize(double, double);
	PageSize& operator=(const PageSize& other);

	const QString& name() const { return m_pageInfo.id; }
	const QString& nameTR() const { return m_pageInfo.displayName; }
	QString categoryId() const { return m_pageInfo.categoryId; }
	double width() const { return m_pageInfo.width; }
	double height() const { return m_pageInfo.height; }
	double originalWidth() const { return width() * unitGetRatioFromIndex(m_pageInfo.pageUnitIndex); }
	double originalHeight() const { return height() * unitGetRatioFromIndex(m_pageInfo.pageUnitIndex); }
	QString originalUnit() const { return unitGetSuffixFromIndex(m_pageInfo.pageUnitIndex); }

private:
	PageSizeInfo m_pageInfo;
};

#endif

