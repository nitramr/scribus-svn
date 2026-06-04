/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef TABLEAREA_H
#define TABLEAREA_H

#include <QHash>
#include <QString>

/**
 * Structural areas a cell can occupy within a table, used to select the
 * conditional cell style that applies to it. Resolution priority runs from
 * the corners (highest) down through edges and banding to the whole-table
 * base (lowest); see PageItem_Table::areaAt().
 */
enum class TableArea
{
	WholeTable,
	BodyCell,
	HeaderRow,
	TotalRow,
	FirstColumn,
	LastColumn,
	BandedRowOdd,
	BandedRowEven,
	BandedColOdd,
	BandedColEven,
	TopLeftCell,
	TopRightCell,
	BottomLeftCell,
	BottomRightCell
};

Q_DECLARE_METATYPE(TableArea)

inline size_t qHash(TableArea area, size_t seed = 0)
{
	return ::qHash(static_cast<int>(area), seed);
}

/**
 * Serialises a TableArea to the string used for the .sla "Type" attribute.
 */
inline QString tableAreaToString(TableArea area)
{
	switch (area)
	{
		case TableArea::BodyCell:
			return QStringLiteral("BodyCell");
		case TableArea::HeaderRow:
			return QStringLiteral("HeaderRow");
		case TableArea::TotalRow:
			return QStringLiteral("TotalRow");
		case TableArea::FirstColumn:
			return QStringLiteral("FirstColumn");
		case TableArea::LastColumn:
			return QStringLiteral("LastColumn");
		case TableArea::BandedRowOdd:
			return QStringLiteral("BandedRowOdd");
		case TableArea::BandedRowEven:
			return QStringLiteral("BandedRowEven");
		case TableArea::BandedColOdd:
			return QStringLiteral("BandedColOdd");
		case TableArea::BandedColEven:
			return QStringLiteral("BandedColEven");
		case TableArea::TopLeftCell:
			return QStringLiteral("TopLeftCell");
		case TableArea::TopRightCell:
			return QStringLiteral("TopRightCell");
		case TableArea::BottomLeftCell:
			return QStringLiteral("BottomLeftCell");
		case TableArea::BottomRightCell:
			return QStringLiteral("BottomRightCell");
		case TableArea::WholeTable:
			return QStringLiteral("WholeTable");
	}
	return QStringLiteral("WholeTable");
}

/**
 * Parses a TableArea from a .sla "Type" attribute. Unknown values (e.g. an
 * area added by a later Scribus version) fail safe to WholeTable.
 */
inline TableArea tableAreaFromString(const QString& s)
{
	if (s == QLatin1String("BodyCell"))
		return TableArea::BodyCell;
	if (s == QLatin1String("HeaderRow"))
		return TableArea::HeaderRow;
	if (s == QLatin1String("TotalRow"))
		return TableArea::TotalRow;
	if (s == QLatin1String("FirstColumn"))
		return TableArea::FirstColumn;
	if (s == QLatin1String("LastColumn"))
		return TableArea::LastColumn;
	if (s == QLatin1String("BandedRowOdd"))
		return TableArea::BandedRowOdd;
	if (s == QLatin1String("BandedRowEven"))
		return TableArea::BandedRowEven;
	if (s == QLatin1String("BandedColOdd"))
		return TableArea::BandedColOdd;
	if (s == QLatin1String("BandedColEven"))
		return TableArea::BandedColEven;
	if (s == QLatin1String("TopLeftCell"))
		return TableArea::TopLeftCell;
	if (s == QLatin1String("TopRightCell"))
		return TableArea::TopRightCell;
	if (s == QLatin1String("BottomLeftCell"))
		return TableArea::BottomLeftCell;
	if (s == QLatin1String("BottomRightCell"))
		return TableArea::BottomRightCell;
	return TableArea::WholeTable;
}

#endif // TABLEAREA_H