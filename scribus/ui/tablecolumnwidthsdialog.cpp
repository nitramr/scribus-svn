/*
Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "pageitem_table.h"

#include "tablecolumnwidthsdialog.h"

TableColumnWidthsDialog::TableColumnWidthsDialog(int unitIndex, double value, QWidget *parent) : QDialog(parent)
{
	setupUi(this);

	m_unitIndex = unitIndex;
	tableColumnWidth->setNewUnit(unitIndex);
	double ur = unitGetRatioFromIndex(unitIndex);
	tableColumnWidth->setMinimum(PageItem_Table::MinimumColumnWidth * ur);
	tableColumnWidth->setValue(value * ur);
}

double TableColumnWidthsDialog::columnWidth() const
{
	return tableColumnWidth->getValue(m_unitIndex);
}
