/*
Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef TABLEROWHEIGHTSDIALOG_H
#define TABLEROWHEIGHTSDIALOG_H

#include "ui_tablerowheightsdialog.h"

#include "units.h"

/**
 * This is the dialog used when setting row heights on a table.
 */
class TableRowHeightsDialog : public QDialog, private Ui::TableRowHeightsDialog
{
	Q_OBJECT
public:
	/// Constructs a new dialog. The dialog will use the unit of @a doc.
	explicit TableRowHeightsDialog(int unitIndex, double value, QWidget* parent = nullptr);

	/// Returns the row height the user entered.
	double rowHeight() const;

private:
	int m_unitIndex {SC_PT};
	double m_value {0.0};
};

#endif // TABLEROWHEIGHTSDIALOG_H
