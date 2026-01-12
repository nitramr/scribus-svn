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

#ifndef MARGINS_H
#define MARGINS_H

#include <QDebug>
#include <QMarginsF>

/** \brief Pagemargins and bleeds*/
class MarginStruct
{
	public:
		MarginStruct() {}
		MarginStruct(double top, double left, double bottom, double right, double all = 0.0) :
			m_top(top), m_left(left), m_bottom(bottom), m_right(right), m_all(all) {}

		void set(double top, double left, double bottom, double right, double all = 0.0) { m_top = top; m_bottom = bottom; m_left = left; m_right = right; m_all = all;}
		void resetToZero() { m_top = 0.0; m_bottom = 0.0; m_left = 0.0; m_right = 0.0; m_all = 0.0; }
		bool isNull() const {return qFuzzyIsNull(m_left) && qFuzzyIsNull(m_top) && qFuzzyIsNull(m_right) && qFuzzyIsNull(m_bottom) && qFuzzyIsNull(m_all);}
		void print() const { qDebug() << m_top << m_left << m_bottom << m_right << m_all; }
		inline double left() const { return m_left; }
		inline double top() const { return m_top; }
		inline double bottom() const { return m_bottom; }
		inline double right() const { return m_right; }
		inline double all() const { return m_all; }

		inline void setLeft(double aleft) { m_left = aleft; }
		inline void setTop(double atop) { m_top = atop; }
		inline void setRight(double aright) { m_right = aright; }
		inline void setBottom(double abottom) { m_bottom = abottom; }
		inline void setAll(double aall) { m_all = aall; }

		bool operator==(const MarginStruct &other) const
		{
			return ((m_left == other.m_left) && (m_top == other.m_top) && (m_right == other.m_right) && (m_bottom == other.m_bottom) && (m_all == other.m_all));
		}
		bool operator!=(const MarginStruct& other) const
		{
			return !((m_left == other.m_left) && (m_top == other.m_top) && (m_right == other.m_right) && (m_bottom == other.m_bottom) && (m_all == other.m_all));
		}

		void fromString(const QString &string)
		{
			resetToZero();
			QStringList s = string.split(" ");
			if (s.size() == 5) // l t r b a
				set(convert(s.at(1)), convert(s.at(0)), convert(s.at(3)), convert(s.at(2)), convert(s.at(4)));
		}

		QString toString() const
		{
			return QString("%0 %1 %2 %3 %4").arg(QString::number(m_left)).arg(QString::number(m_top)).arg(QString::number(m_right)).arg(QString::number(m_bottom)).arg(QString::number(m_all));
		}

	protected:
		double m_top {0.0};
		double m_left {0.0};
		double m_bottom {0.0};
		double m_right {0.0};
		double m_all {0.0};

		double convert(const QString& s)
		{
			bool ok(false);
			double number = s.toDouble(&ok);
			return ok ? number : 0.0;
		}
};

class ScMargins : public QMarginsF
{
	public:
		ScMargins(qreal left, qreal top, qreal right, qreal bottom) : QMarginsF(left, top, right, bottom) {};
		void set(qreal left, qreal top, qreal right, qreal bottom)
		{
			setTop(top);
			setBottom(bottom);
			setLeft(left);
			setRight(right);
		}
};

#endif // MARGINS_H
