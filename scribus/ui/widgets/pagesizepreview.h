#ifndef PAGESIZEPREVIEW_H
#define PAGESIZEPREVIEW_H

#include <QWidget>

#include "margins.h"
#include "manager/pagepreset_manager.h"

class PageSizePreview : public QWidget
{
	Q_OBJECT

public:
	explicit PageSizePreview(QWidget *parent = nullptr);

	void setPageHeight(double height)
	{
		PageSizeInfo psi = PagePresetManager::instance().pageInfoByDimensions(QSizeF(m_width, height));
		m_name = psi.displayName;
		m_height = height;
		update();
	};
	void setPageWidth(double width) { m_width = width; update(); };
	void setMargins(const MarginStruct& margins) { m_margins = margins; update(); };
	void setBleeds(const MarginStruct& bleeds) { m_bleeds = bleeds; update(); };
	void setLayout(int layout) { m_layout = layout; update(); };
	void setFirstPage(int firstPage) { m_firstPage = firstPage; update(); };
	void setPage(double height, double width, const MarginStruct& margins, const MarginStruct& bleeds, int layout, int firstPage)
	{
		PageSizeInfo psi = PagePresetManager::instance().pageInfoByDimensions(QSizeF(width, height));

		m_height = height;
		m_width = width;
		m_margins = margins;
		m_bleeds = bleeds;
		m_name = psi.displayName;
		m_layout = layout;
		m_firstPage = firstPage;
		update();
	}

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	MarginStruct m_margins {};
	MarginStruct m_bleeds {};
	double m_height { 1.0 };
	double m_width { 1.0 };
	QString m_name;
	int m_layout { 0 };
	int m_firstPage { 0 };
};

#endif // PAGESIZEPREVIEW_H
