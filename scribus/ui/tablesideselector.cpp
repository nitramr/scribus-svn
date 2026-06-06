/*
Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include <algorithm>

#include <QColor>
#include <QDebug>
#include <QFrame>
#include <QLineF>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPair>
#include <QPen>
#include <QPointF>
#include <QRectF>

#include "tablesideselector.h"

class QEvent;

TableSideSelector::TableSideSelector(QWidget* parent) : QFrame(parent)
{
	setSelection(TableSide::All);
	setStyle(TableStyle);
	setFrameShape(QFrame::StyledPanel);
	setFrameShadow(QFrame::Sunken);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setMouseTracking(true);
}

QList<TableSide> TableSideSelector::selectionList() const
{
	QList<TableSide> sides;
	if (m_selection & TableSide::Left)
		sides.append(TableSide::Left);
	if (m_selection & TableSide::Right)
		sides.append(TableSide::Right);
	if (m_selection & TableSide::Top)
		sides.append(TableSide::Top);
	if (m_selection & TableSide::Bottom)
		sides.append(TableSide::Bottom);
	return sides;
}

void TableSideSelector::setInnerActive(bool active)
{
	if (m_innerActive == active)
		return;
	m_innerActive = active;
	update();
}

void TableSideSelector::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	double edgeWidth = 5;
	double inset = edgeWidth/2 + frameWidth()*2;

	QPointF topLeft(inset, inset);
	QPointF topRight(width() - inset, inset);
	QPointF bottomLeft(inset, height() - inset);
	QPointF bottomRight(width() - inset, height() - inset);

	m_left = QLineF(topLeft, bottomLeft);
	m_right = QLineF(topRight, bottomRight);
	m_top = QLineF(topRight, topLeft);
	m_bottom = QLineF(bottomRight, bottomLeft);
	m_innerHorizontal = QLineF(m_left.pointAt(0.5), m_right.pointAt(0.5));
	m_innerVertical = QLineF(m_top.pointAt(0.5), m_bottom.pointAt(0.5));

	QPainter painter(this);

	// Paint outer-side selection.
	painter.setPen(QPen(Qt::black, edgeWidth, Qt::SolidLine, Qt::RoundCap));
	if (m_selection & TableSide::Left)
		painter.drawLine(m_left);
	if (m_selection & TableSide::Right)
		painter.drawLine(m_right);
	if (m_selection & TableSide::Top)
		painter.drawLine(m_top);
	if (m_selection & TableSide::Bottom)
		painter.drawLine(m_bottom);

	// Paint inner-cross selection (only when active).
	if (m_innerActive)
	{
		if (m_selection & TableSide::InnerHorizontal)
			painter.drawLine(m_innerHorizontal);
		if (m_selection & TableSide::InnerVertical)
			painter.drawLine(m_innerVertical);
	}

	// Paint dotted grid: outer rectangle (always).
	painter.setPen(QPen(Qt::gray, edgeWidth/4, Qt::DotLine));
	painter.drawRect(QRectF(topLeft, bottomRight));

	// Paint dotted inner cross only when the inner zone is active. When
	// inactive (e.g. table-style editing, which has no interior borders),
	// drawing it would misleadingly imply an editable inner border.
	if (m_innerActive)
	{
		painter.setPen(QPen(Qt::gray, edgeWidth/4, Qt::DotLine));
		painter.drawLine(m_innerHorizontal);
		painter.drawLine(m_innerVertical);
	}

	// Highlighting (hover indication).
	painter.setPen(QPen(QColor(255, 255, 255, 60), edgeWidth, Qt::SolidLine, Qt::RoundCap));
	switch (m_highlighted)
	{
		case TableSide::Left:
			painter.drawLine(m_left);
			break;
		case TableSide::Right:
			painter.drawLine(m_right);
			break;
		case TableSide::Top:
			painter.drawLine(m_top);
			break;
		case TableSide::Bottom:
			painter.drawLine(m_bottom);
			break;
		case TableSide::InnerHorizontal:
			if (m_innerActive)
				painter.drawLine(m_innerHorizontal);
			break;
		case TableSide::InnerVertical:
			if (m_innerActive)
				painter.drawLine(m_innerVertical);
			break;
		default: break;
	}
}

void TableSideSelector::mousePressEvent(QMouseEvent* event)
{
	m_selection ^= closestSide(event->position());
	update();
	emit selectionChanged();
}

void TableSideSelector::mouseMoveEvent(QMouseEvent* event)
{
	m_highlighted = closestSide(event->position());
	update();
}

void TableSideSelector::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	m_highlighted = TableSide::None;
	update();
}

TableSide TableSideSelector::closestSide(const QPointF& point) const
{
	QList<QPair<double, TableSide>> distances;
	distances.append({distanceToSegment(point, m_left),   TableSide::Left});
	distances.append({distanceToSegment(point, m_right),  TableSide::Right});
	distances.append({distanceToSegment(point, m_top),    TableSide::Top});
	distances.append({distanceToSegment(point, m_bottom), TableSide::Bottom});

	if (m_innerActive)
	{
		// For hit-testing, use a shorter segment so clicks near where the
		// inner cross meets an outer edge unambiguously belong to the
		// outer side.
		const double margin = 0.2;
		QLineF hitH(m_innerHorizontal.pointAt(margin), m_innerHorizontal.pointAt(1.0 - margin));
		QLineF hitV(m_innerVertical.pointAt(margin), m_innerVertical.pointAt(1.0 - margin));
		distances.append({distanceToSegment(point, hitH), TableSide::InnerHorizontal});
		distances.append({distanceToSegment(point, hitV), TableSide::InnerVertical});
	}

	std::sort(distances.begin(), distances.end());
	return distances.first().second;
}

double TableSideSelector::distanceToSegment(const QPointF& point, const QLineF& line)
{
	QPointF a = line.p1();
	QPointF b = line.p2();
	double dx = b.x() - a.x();
	double dy = b.y() - a.y();
	double lengthSquared = dx * dx + dy * dy;
	if (lengthSquared == 0)
		return QLineF(point, a).length();
	double t = ((point.x() - a.x()) * dx + (point.y() - a.y()) * dy) / lengthSquared;
	t = qBound(0.0, t, 1.0);
	QPointF closest(a.x() + t * dx, a.y() + t * dy);
	return QLineF(point, closest).length();
}
