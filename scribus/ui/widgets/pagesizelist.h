/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef PAGESIZELIST_H
#define PAGESIZELIST_H

#include <QListView>
#include <QScrollBar>
#include <QStandardItemModel>

#include "scribusapi.h"

class SCRIBUS_API PageSizeList : public QListView
{
	Q_OBJECT

public:

	enum SortMode
	{
		NameAsc = 0,
		NameDesc = 1,
		DimensionAsc = 2,
		DimensionDesc = 3
	};

	enum ItemData
	{
		SizeLabel = Qt::UserRole,
		Type = Qt::UserRole + 1,
		Unit = Qt::UserRole + 2,
		Category = Qt::UserRole + 3,
		ID = Qt::UserRole + 4,
		Dimension = Qt::UserRole + 5,
		Width = Qt::UserRole + 6,
		Height = Qt::UserRole + 7,
		Name = Qt::UserRole + 8,
		MarginPreset = Qt::UserRole + 9,
		Margins = Qt::UserRole + 10,
		Bleeds = Qt::UserRole + 11,
		Layout = Qt::UserRole + 12,
		FirstPage = Qt::UserRole + 13,
		TextFrame = Qt::UserRole + 14
	};

	PageSizeList(QWidget* parent);
	~PageSizeList() = default;

	void setDimensions(double width, double height);

	void setOrientation(int orientation);
	int orientation() const { return m_orientation; };

	void setCategory(const QString& category);
	const QString& category() const { return m_category; };

	void setSortMode(SortMode sortMode);
	SortMode sortMode() const { return m_sortMode; };

	void setValues(QSizeF dimensions, int orientation, const QString& category, SortMode sortMode);

	void updateGeometries() override;

private slots:

	void showContextMenu(const QPoint &pos);
	void deleteItem();

private:
	QSizeF m_dimensions;
	int m_orientation {0};
	QString m_category;
	SortMode m_sortMode {SortMode::NameAsc};
	QStandardItemModel* m_model { nullptr };
	QModelIndex m_modelIndex;

	QIcon sizePreview(QSize iconSize, QSize pageSize, QList<double> dataMargins) const;
	void loadPageSizes(QSizeF dimensions, int orientation, const QString& category);

signals:
	void changedCategories();
};


#endif // PAGESIZELIST_H
