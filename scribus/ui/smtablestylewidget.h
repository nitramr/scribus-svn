/*
Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#ifndef SMTABLESTYLEWIDGET_H
#define SMTABLESTYLEWIDGET_H

#include <QWidget>

#include "ui_smtablestylewidget.h"
#include "styles/tablearea.h"
#include "styles/tablestyle.h"
#include "tableborder.h"
#include "tablesideselector.h"

/**
 * Widget for editing table style attributes.
 *
 * NOTE: Many attributes unsupported.
 */
class SMTableStyleWidget : public QWidget, public Ui::SMTableStyleWidget
{
		Q_OBJECT
	public:
		/// Constructor.
		SMTableStyleWidget(QWidget* parent = nullptr);
		/// Destructor.
		~SMTableStyleWidget() = default;

		void setDoc(ScribusDoc* doc);

		/**
		 * Shows attributes for a single table style.
		 *
		 * @param tableStyle table style for which attributes should be shown.
		 * @param tableStyles list of all table styles.
		 * @param defaultLanguage default language.
		 * @param unitIndex index of currently used unit.
		 */
		void show(TableStyle *tableStyle, QList<TableStyle> &tableStyles, const QString &defaultLanguage, int unitIndex);

		/**
		 * Shows attributes for multiple table styles.
		 *
		 * TODO: Implement actual support for multiple styles.
		 *
		 * @param tableStyles list of table styles for which attributes should be shown.
		 * @param tableStylesAll list of all table styles.
		 * @param defaultLanguage default language.
		 * @param unitIndex index of currently used unit.
		 */
		void show(QList<TableStyle*> &tableStyles, QList<TableStyle> &tableStylesAll, const QString &defaultLanguage, int unitIndex);

		/**
		 * This function is called when the language is changed.
		 */
		void languageChange();

		/**
		 * Populates the fill color combo with the given color list.
		 *
		 * @param colors list of colors to populate the combo with.
		 */
		void fillFillColorCombo(ColorList &colors);
		void showColors(const QList<TableStyle*> &tableStyles);
		void setBorders(const TableBorder& left, const TableBorder& right, const TableBorder& top, const TableBorder& bottom);
		TableArea currentArea() const { return m_currentArea; }
		void rebuildAreaCombo(TableStyle *tableStyle);
		void showFillForCurrentArea(TableStyle *tableStyle);
		void showParagraphStyleForCurrentArea(TableStyle *tableStyle);

	signals:
		// Emitted when the user changes the border on one or more sides.
		void bordersChanged(TableSides sides, const TableBorder& border);
		void conditionalAreaChanged(TableArea area);

	protected:
		void changeEvent(QEvent *e) override;

	private:
		enum State { Unset, Set, TriState };

		ScribusDoc * m_Doc = nullptr;

		// Border-list state (mirrors the PP version, simplified for SM).
		TableBorder m_currentBorder;
		TableSides m_lastSelection { TableSide::All };

		// Per-side state, set via setBorders, read by the slots.
		TableBorder m_leftBorder;
		TableBorder m_rightBorder;
		TableBorder m_topBorder;
		TableBorder m_bottomBorder;
		TableArea m_currentArea { TableArea::WholeTable };

		void updateBorderLineList();
		void updateBorderLineListItem();
		void mirrorCurrentBorderToSelectedSides();
		QColor getColor(const QString& colorName, int shade) const;
		void setCheck(QCheckBox* cb, bool v);
		void setSpin(QSpinBox* sb, int v, bool enabled = true);

	private slots:
		void handleUpdateRequest(int);
		void iconSetChange();
		void on_sideSelector_selectionChanged();
		void on_borderLineList_currentRowChanged(int row);
		void on_addBorderLineButton_clicked();
		void on_removeBorderLineButton_clicked();
		void on_borderLineWidth_valueChanged(double width);
		void on_borderLineShade_valueChanged(double shade);
		void on_borderLineColor_textActivated(const QString& colorName);
		void on_borderLineStyle_activated(int style);
		void on_conditionalAreaComboBox_currentIndexChanged(int index);
};

#endif // SMTABLESTYLEWIDGET_H
