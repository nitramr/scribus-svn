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

#include "pagesize.h"
#include <QStringList>
#include <QObject>

PageSize::PageSize(const QString& sizeName)
{
	m_pageInfo = PagePresetManager::instance().pageInfoByName(sizeName);
}

PageSize::PageSize(double w, double h)
{
	m_pageInfo = PagePresetManager::instance().pageInfoByDimensions(QSizeF(w, h));
}

PageSize& PageSize::operator=(const PageSize& other)
{
	m_pageInfo = PagePresetManager::instance().pageInfoByDimensions(QSizeF(other.width(), other.height()));
	return *this;
}
