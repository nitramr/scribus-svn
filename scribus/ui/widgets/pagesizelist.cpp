/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include <QApplication>
#include <QPainter>

#include "pagesizelist.h"
#include "prefsmanager.h"
#include "manager/pagepreset_manager.h"
#include "ui/delegates/sclistitemdelegate.h"


PageSizeList::PageSizeList(QWidget* parent) :
	QListView(parent)
{
	m_model = new QStandardItemModel();
	setModel(m_model);
	setDragEnabled(false);
	setViewMode(QListView::IconMode);
	setFlow(QListView::LeftToRight);
	setWordWrap(true);
	setAcceptDrops(false);
	setDropIndicatorShown(false);
	setDragDropMode(QAbstractItemView::NoDragDrop);
	setResizeMode(QListView::Adjust);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setFocusPolicy(Qt::NoFocus);
	setIconSize(QSize(80, 80));
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
#ifdef Q_OS_MACOS
	setGridSize(QSize(150, 150));
#else
	setGridSize(QSize(160, 160));
#endif
	setItemDelegate(new ScListItemDelegate(QListWidget::IconMode, iconSize()));
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, &QListView::customContextMenuRequested, this, &PageSizeList::showContextMenu);
}

void PageSizeList::setDimensions(double width, double height)
{
	loadPageSizes(QSizeF(width, height), m_orientation, m_category);
	m_dimensions = QSizeF(width, height);
	setSortMode(m_sortMode);
}

void PageSizeList::setOrientation(int orientation)
{
	loadPageSizes(m_dimensions, orientation, m_category);
	m_orientation = orientation;
	setSortMode(m_sortMode);
}

void PageSizeList::setCategory(const QString& category)
{
	loadPageSizes(m_dimensions, m_orientation, category);
	m_category = category;
	setSortMode(m_sortMode);
}

void PageSizeList::setSortMode(SortMode sortMode)
{
	QSignalBlocker sig(this);

	m_sortMode = sortMode;

	switch (sortMode)
	{
	case NameAsc:
		m_model->setSortRole(ItemData::Name);
		m_model->sort(0, Qt::AscendingOrder);
		break;
	case NameDesc:
		m_model->setSortRole(ItemData::Name);
		m_model->sort(0, Qt::DescendingOrder);
		break;
	case DimensionAsc:
		m_model->setSortRole(ItemData::Dimension);
		m_model->sort(0, Qt::AscendingOrder);
		break;
	case DimensionDesc:
		m_model->setSortRole(ItemData::Dimension);
		m_model->sort(0, Qt::DescendingOrder);
		break;
	}
}

void PageSizeList::setValues(QSizeF dimensions, int orientation, const QString& category, SortMode sortMode)
{
	loadPageSizes(dimensions, orientation, category);
	m_dimensions = dimensions;
	m_orientation = orientation;
	m_category = category;
	setSortMode(sortMode);
}

void PageSizeList::loadPageSizes(QSizeF dimensions, int orientation, const QString& category)
{
	QSignalBlocker sig(this);

	auto &ppm = PagePresetManager::instance();

	PageSizeInfo pref = ppm.pageInfoByDimensions(PrefsManager::instance().appPrefs.docSetupPrefs.pageWidth, PrefsManager::instance().appPrefs.docSetupPrefs.pageHeight);
	PageSizeInfo ps = ppm.pageInfoByDimensions(dimensions.width(), dimensions.height());

	int sel = -1;

	// reset sorting to default
	m_model->setSortRole(ItemData::Name);
	m_model->sort(0, Qt::AscendingOrder);

	// enable if list selection should be remembered
	// if (m_category == category && this->selectionModel()->currentIndex().isValid())
	// 	sel = this->selectionModel()->currentIndex().row();

	m_model->clear();

	PageCollectionInfo pciPreferred = ppm.categoryInfoPreferred();

	foreach (auto item, ppm.pageSizes())
	{
		QSize size;
		size.setWidth(orientation == 0 ? item.width : item.height);
		size.setHeight(orientation == 0 ? item.height : item.width);

		// Add items of selected category or all preferred and defaults
		if (item.categoryId == category ||
				(category == pciPreferred.id && ppm.activePageSizes().contains(item.id)) ||
				(category == pciPreferred.id && item.id == pref.id))
		{
			QList<double> margins;
			margins.append(item.margins.isNull());
			margins.append(item.margins.top());
			margins.append(item.margins.left());
			margins.append(item.margins.bottom());
			margins.append(item.margins.right());

			QList<double> bleeds;
			bleeds.append(item.bleeds.isNull());
			bleeds.append(item.bleeds.top());
			bleeds.append(item.bleeds.left());
			bleeds.append(item.bleeds.bottom());
			bleeds.append(item.bleeds.right());

			QStandardItem* itemA = new QStandardItem();
			itemA->setText(item.displayName);
			itemA->setEditable(false);
			itemA->setIcon(sizePreview(this->iconSize(), size, margins));
			itemA->setData(QVariant(item.label), ItemData::SizeLabel);
			itemA->setData(QVariant(item.pageUnitIndex), ItemData::Unit);
			itemA->setData(QVariant(item.categoryId), ItemData::Category);
			itemA->setData(QVariant(item.id), ItemData::ID);
			itemA->setData(QVariant(item.width * item.height), ItemData::Dimension);
			itemA->setData(QVariant(item.width), ItemData::Width);
			itemA->setData(QVariant(item.height), ItemData::Height);
			itemA->setData(QVariant(item.type), ItemData::Type);
			itemA->setData(QVariant(item.displayName), ItemData::Name);
			itemA->setData(QVariant(item.marginPreset), ItemData::MarginPreset);
			itemA->setData(QVariant::fromValue(margins), ItemData::Margins);
			itemA->setData(QVariant::fromValue(bleeds), ItemData::Bleeds);
			itemA->setData(QVariant(item.layout), ItemData::Layout);
			itemA->setData(QVariant(item.firstPage), ItemData::FirstPage);
			itemA->setData(QVariant::fromValue(item.textFrame), ItemData::TextFrame);

			m_model->appendRow(itemA);

			// select item with name match OR equal size
			if (sel == -1 && (item.id == ps.id || (item.width == ps.width && item.height == ps.height)))
				sel = itemA->row();

		}
	}

	if (sel > -1)
		this->selectionModel()->setCurrentIndex(m_model->item(sel)->index(), QItemSelectionModel::Select );
}

void PageSizeList::updateGeometries()
{
	QListView::updateGeometries();
	verticalScrollBar()->setSingleStep(10);
}

QIcon PageSizeList::sizePreview(QSize iconSize, QSize pageSize, QList<double> dataMargins) const
{
	double devicePixelRatio = qApp->devicePixelRatio();
	double max = mm2pts(500 * devicePixelRatio); // reference for scale: large side of B3
	double iW = iconSize.width() * devicePixelRatio;
	double iH = iconSize.height() * devicePixelRatio;
	double pW = pageSize.width() * devicePixelRatio;
	double pH = pageSize.height() * devicePixelRatio;
	bool isVertical = pW <= pH;

	QPixmap pix(QSize(iW, iH));
	pix.fill(Qt::transparent);

	// vertical
	double mod = qMin(pH / max, 1.0);
	double ratio = pW / pH;
	int height = iH * mod;
	int width = height * ratio;
	int x = (iW - width) / 2;
	int y = iH - height;

	// horizontal
	if (!isVertical)
	{
		mod = qMin(pW / max, 1.0);
		ratio = pH / pW;
		width = iW * mod;
		height = width * ratio;
		x = (iW - width) / 2;
		y = iH - height;
	}

	QRect page(x, y, width, height);

	QLinearGradient m_gradient(page.topLeft(), page.bottomRight());
	m_gradient.setColorAt(0.0, Qt::white);
	m_gradient.setColorAt(1.0, QColor(240, 240, 240));

	QPainter painter(&pix);
	painter.setPen(QPen(palette().text().color()));
	painter.setBrush(m_gradient);
	painter.drawRect(page.adjusted(0, 0, -1, -1));

	if (!QVariant(dataMargins.at(0)).toBool())
	{

		int t = ceil(dataMargins.at(1) / height * devicePixelRatio);
		int l = ceil(dataMargins.at(2) / width * devicePixelRatio);
		int b = ceil(dataMargins.at(3) / height * devicePixelRatio);
		int r = ceil(dataMargins.at(4) / width * devicePixelRatio);

		QColor colMargin = PrefsManager::instance().appPrefs.guidesPrefs.marginColor;
		painter.setPen(colMargin);
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(page.adjusted(l, t, -r - 1, -b - 1));
	}

	painter.end();

	return QIcon(pix);
}

void PageSizeList::showContextMenu(const QPoint &pos)
{
	QModelIndex index = indexAt(pos);

	if (!index.isValid())
		return;

	m_modelIndex = index;

	int itemType = index.data(ItemData::Type).toInt();

	QMenu contextMenu(this);
	QAction *deleteAction = contextMenu.addAction(tr("Delete Preset"));

	if (itemType == PageSizeType::User)
		deleteAction->setEnabled(true);
	else
	{
		deleteAction->setEnabled(false);
		deleteAction->setToolTip("This preset cannot be deleted");
	}

	connect(deleteAction, &QAction::triggered, this, &PageSizeList::deleteItem);

	contextMenu.exec(mapToGlobal(pos));
}

void PageSizeList::deleteItem()
{
	if (m_modelIndex.isValid())
	{
		QString categoryId = m_modelIndex.data(PageSizeList::Category).toString();
		QString pageId = m_modelIndex.data(PageSizeList::ID).toString();
		PageCollectionInfo pci = PagePresetManager::instance().categoryInfoById(categoryId);

		PagePresetManager::instance().removeCollectionPage(pci.filePath, pageId);
		model()->removeRow(m_modelIndex.row());

		bool isEmpty = PagePresetManager::instance().isCollectionsEmpty(pci.filePath);
		if (isEmpty)
			PagePresetManager::instance().removeCollection(pci.filePath);

		PagePresetManager::instance().reloadAllPresets();

		if (isEmpty)
			emit changedCategories();
	}
}
