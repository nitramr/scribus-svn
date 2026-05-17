/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QVBoxLayout>

#include "pagesizeselector.h"

PageSizeSelector::PageSizeSelector(QWidget *parent)
	: QWidget{parent}
{
	comboCategory = new QComboBox();
	comboFormat = new QComboBox();

	QVBoxLayout* lytMain = new QVBoxLayout();
	lytMain->setContentsMargins(0, 0, 0, 0);
	lytMain->setSpacing(4);
	lytMain->addWidget(comboCategory);
	lytMain->addWidget(comboFormat);
	this->setLayout(lytMain);

	connect(comboCategory, &QComboBox::currentIndexChanged, this, &PageSizeSelector::categorySelected);
	connect(comboFormat, &QComboBox::currentIndexChanged, this, &PageSizeSelector::formatSelected);
}


void PageSizeSelector::setHasFormatSelector(bool isVisble)
{
	m_hasFormatSelector = isVisble;
	comboFormat->setVisible(isVisble);
}

void PageSizeSelector::setHasCustom(bool hasCustom)
{
	m_hasCustom = hasCustom;

	if (!m_sizeName.isEmpty())
		setPageSize(m_size.width(), m_size.height());
}

void PageSizeSelector::setCurrentCategory(const QString &categoryId)
{
	int index = comboCategory->findData(categoryId);
	if (index != -1)
		comboCategory->setCurrentIndex(index);
}

void PageSizeSelector::setup(PageSizeInfo psi)
{
	m_sizeName = psi.id;
	m_sizeCategory = psi.categoryId;
	m_trSizeName = psi.displayName;

	// Load category list
	int index = -1;
	QSignalBlocker sigCat(comboCategory);
	comboCategory->clear();

	// Add Custom
	if (hasCustom())
	{
		PageCollectionInfo pciCustom = PagePresetManager::instance().categoryInfoCustom();
		comboCategory->addItem(pciCustom.displayName, pciCustom.id);
		if (m_sizeName == pciCustom.id || m_sizeName == pciCustom.displayName)
			index = comboCategory->count() - 1;
	}

	// Add Preferred
	PageCollectionInfo pciPreferred = PagePresetManager::instance().categoryInfoPreferred();
	comboCategory->addItem(pciPreferred.displayName, pciPreferred.id);
	comboCategory->insertSeparator(comboCategory->count());

	// Add all available categories
	QList<QString> orderList = PagePresetManager::instance().categoriesOrder();
	for (int i = 0; i < orderList.size(); i++)
	{
		QString item = orderList.at(i);
		if (item == "-")
			comboCategory->insertSeparator(comboCategory->count());
		else
		{
			PageCollectionInfo pci = PagePresetManager::instance().categoryInfoById(item);
			comboCategory->addItem(pci.displayName, pci.id);
			if (pci.id == m_sizeCategory)
				index = comboCategory->count() - 1;
		}

	}

	comboCategory->setCurrentIndex(index);

	// Load size format list
	setFormat(m_sizeCategory, m_sizeName);
}

void PageSizeSelector::setPageSize(double width, double height)
{
	m_size = QSizeF(width, height);
	PageSizeInfo psi = PagePresetManager::instance().pageInfoByDimensions(m_size);
	setup(psi);
}

void PageSizeSelector::setFormat(const QString& categoryId, QString name)
{
	if (!hasFormatSelector())
		return;

	QSignalBlocker sigFormat(comboFormat);
	comboFormat->clear();

	PageCollectionInfo pciCustom = PagePresetManager::instance().categoryInfoCustom();

	if (categoryId == pciCustom.id)
	{
		comboFormat->setEnabled(false);
		m_sizeName = pciCustom.id;
		m_trSizeName = pciCustom.displayName;
		return;
	}
	else
	{
		comboFormat->setEnabled(true);
	}

	// PageSize ps(name);
	PageCollectionInfo pciPreferred = PagePresetManager::instance().categoryInfoPreferred();
	int index = -1;
	for (const auto &item : PagePresetManager::instance().pageSizes())
	{
		if (item.categoryId == categoryId || (categoryId == pciPreferred.id && PagePresetManager::instance().activePageSizes().contains(item.id)))
		{
			comboFormat->addItem(item.displayName, item.id);
			if (item.id == name)
				index = comboFormat->count() - 1;
		}
	}

	if (index == -1)
		index = 0;

	comboFormat->setCurrentIndex(index);

	m_sizeName = comboFormat->currentData().toString();
	m_trSizeName = comboFormat->currentText();
}

void PageSizeSelector::categorySelected(int index)
{
	m_sizeCategory = comboCategory->itemData(index).toString();

	setFormat(m_sizeCategory, m_sizeName);
	emit pageCategoryChanged(m_sizeCategory);
	emit pageSizeChanged(m_trSizeName);
}

void PageSizeSelector::formatSelected(int index)
{
	m_sizeName = comboFormat->itemData(index).toString();
	m_trSizeName = comboFormat->itemText(index);

	emit pageSizeChanged(m_trSizeName);
}
