/*
Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "pageitem_table.h"

#include "tablerowheightsdialog.h"

TableRowHeightsDialog::TableRowHeightsDialog(int unitIndex, double value, QWidget *parent) : QDialog(parent)
{
	setupUi(this);

	m_unitIndex = unitIndex;
	tableRowHeight->setNewUnit(unitIndex);
	double ur = unitGetRatioFromIndex(unitIndex);
	tableRowHeight->setMinimum(PageItem_Table::MinimumRowHeight * ur);
	tableRowHeight->setValue(value * ur);
}

double TableRowHeightsDialog::rowHeight() const
{
	return tableRowHeight->getValue(m_unitIndex);
}
