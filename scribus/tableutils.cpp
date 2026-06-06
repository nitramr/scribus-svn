/*
Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QPointF>

#include "pageitem_table.h"
#include "styles/cellstyle.h"
#include "tableborder.h"
#include "tableutils.h"

namespace TableUtils
{

	// Area priority rank for border-collapse contests. Higher wins. Mirrors the
	// precedence in PageItem_Table::areaAt: corners > columns > header/total >
	// banding > body cell > whole table. A more-specific area's border wins a
	// contested seam outright, regardless of width.
	static int cellAreaPriority(const TableCell& cell, PageItem_Table* table)
	{
		if (!cell.isValid() || !table)
			return 0;

		switch (table->areaAt(cell.row(), cell.column()))
		{
			case TableArea::TopLeftCell:
			case TableArea::TopRightCell:
			case TableArea::BottomLeftCell:
			case TableArea::BottomRightCell:
				return 60;
			case TableArea::FirstColumn:
			case TableArea::LastColumn:
				return 50;
			case TableArea::HeaderRow:
			case TableArea::TotalRow:
				return 40;
			case TableArea::BandedRowOdd:
			case TableArea::BandedRowEven:
			case TableArea::BandedColOdd:
			case TableArea::BandedColEven:
				return 30;
			case TableArea::BodyCell:
				return 20;
			case TableArea::WholeTable:
			default:
				return 10;
		}
	}

	/**
	 * Collapses two cell borders, preferring the explicitly-set border over an
	 * inherited one when the borders would otherwise tie. The border that wins
	 * the tie is the one that the user set, matching the
	 * per-cell border model.
	 */
	TableBorder collapseBordersBetweenCells(
			const TableBorder& firstBorder, bool firstInh, int firstPriority,
			const TableBorder& secondBorder, bool secondInh, int secondPriority)
	{
		// If one side has been explicitly cleared (set to null but not inherited)
		// and the other is inherited, the explicit clear wins -- the user has
		// expressed intent to have no border there.
		if (!firstInh && firstBorder.isNull() && secondInh)
			return firstBorder;
		if (!secondInh && secondBorder.isNull() && firstInh)
			return secondBorder;

		// A more-specific area's border wins a contested seam outright,
		// regardless of width (e.g. header beats body, total beats body).
		// Equal priority falls through to the width/explicit rules below.
		if (firstPriority != secondPriority)
			return (firstPriority > secondPriority) ? firstBorder : secondBorder;

		// Otherwise, prefer explicitly-set over inherited when widths tie.
		if (firstInh && !secondInh)
			return collapseBorders(secondBorder, firstBorder);
		return collapseBorders(firstBorder, secondBorder);
	}

	/**
	 * Collapses a cell border against the table-level fallback border, preferring
	 * the cell border when it was explicitly set. When the cell border is
	 * inherited, the table border is preferred so explicit changes to the
	 * table-level border are visible at the perimeter.
	 */
	TableBorder collapseBorderAgainstTable(
			const TableBorder& cellBorder, bool cellInh,
			const TableBorder& tableBorder)
	{
		// Explicit null cell border wins over the table fallback.
		if (!cellInh && cellBorder.isNull())
			return cellBorder;

		if (cellInh)
			return collapseBorders(tableBorder, cellBorder);
		return collapseBorders(cellBorder, tableBorder);
	}


	void resolveBordersHorizontal(const TableCell& topLeftCell, const TableCell& topCell,
								  const TableCell& topRightCell, const TableCell& bottomLeftCell, const TableCell& bottomCell,
								  const TableCell& bottomRightCell, TableBorder* topLeft, TableBorder* left, TableBorder* bottomLeft,
								  TableBorder* center, TableBorder* topRight, TableBorder* right, TableBorder* bottomRight, PageItem_Table* table)
	{
		// Resolve top left
		if (!topCell.isValid() && !bottomCell.isValid())
			return;
		if (topLeftCell.column() == topCell.column())
			*topLeft = TableBorder();
		else if (topLeftCell.isValid() && topCell.isValid())
			*topLeft = collapseBordersBetweenCells(topCell.leftBorder(), topCell.style().isInhLeftBorder(), cellAreaPriority(topCell, table), topLeftCell.rightBorder(), topLeftCell.style().isInhRightBorder(), cellAreaPriority(topLeftCell, table));
		else if (topLeftCell.isValid())
			*topLeft = collapseBorderAgainstTable(topLeftCell.rightBorder(), topLeftCell.style().isInhRightBorder(), table->rightBorder());
		else if (topCell.isValid())
			*topLeft = collapseBorderAgainstTable(topCell.leftBorder(), topCell.style().isInhLeftBorder(), table->leftBorder());
		else
			*topLeft = TableBorder();

		// Resolve left
		if (topLeftCell.row() == bottomLeftCell.row())
			*left = TableBorder();
		else if (topLeftCell.isValid() && bottomLeftCell.isValid())
			*left = collapseBordersBetweenCells(bottomLeftCell.topBorder(), bottomLeftCell.style().isInhTopBorder(), cellAreaPriority(bottomLeftCell, table), topLeftCell.bottomBorder(), topLeftCell.style().isInhBottomBorder(), cellAreaPriority(topLeftCell, table));
		else if (topLeftCell.isValid())
			*left = collapseBorderAgainstTable(topLeftCell.bottomBorder(), topLeftCell.style().isInhBottomBorder(), table->bottomBorder());
		else if (bottomLeftCell.isValid())
			*left = collapseBorderAgainstTable(bottomLeftCell.topBorder(), bottomLeftCell.style().isInhTopBorder(), table->topBorder());
		else
			*left = TableBorder();

		// Resolve bottom left
		if (bottomLeftCell.column() == bottomCell.column())
			*bottomLeft = TableBorder();
		else if (bottomLeftCell.isValid() && bottomCell.isValid())
			*bottomLeft = collapseBordersBetweenCells(bottomCell.leftBorder(), bottomCell.style().isInhLeftBorder(), cellAreaPriority(bottomCell, table), bottomLeftCell.rightBorder(), bottomLeftCell.style().isInhRightBorder(), cellAreaPriority(bottomLeftCell, table));
		else if (bottomLeftCell.isValid())
			*bottomLeft = collapseBorderAgainstTable(bottomLeftCell.rightBorder(), bottomLeftCell.style().isInhRightBorder(), table->rightBorder());
		else if (bottomCell.isValid())
			*bottomLeft = collapseBorderAgainstTable(bottomCell.leftBorder(), bottomCell.style().isInhLeftBorder(), table->leftBorder());
		else
			*bottomLeft = TableBorder();

		// Resolve center
		if (topCell.row() == bottomCell.row())
			*center = TableBorder();
		else if (topCell.isValid() && bottomCell.isValid())
			*center = collapseBordersBetweenCells(topCell.bottomBorder(), topCell.style().isInhBottomBorder(), cellAreaPriority(topCell, table), bottomCell.topBorder(), bottomCell.style().isInhTopBorder(), cellAreaPriority(bottomCell, table));
		else if (topCell.isValid())
			*center = collapseBorderAgainstTable(topCell.bottomBorder(), topCell.style().isInhBottomBorder(), table->bottomBorder());
		else if (bottomCell.isValid())
			*center = collapseBorderAgainstTable(bottomCell.topBorder(), bottomCell.style().isInhTopBorder(), table->topBorder());
		else
			*center = TableBorder();

		// Resolve top right
		if (topRightCell.column() == topCell.column())
			*topRight = TableBorder();
		else if (topRightCell.isValid() && topCell.isValid())
			*topRight = collapseBordersBetweenCells(topRightCell.leftBorder(), topRightCell.style().isInhLeftBorder(), cellAreaPriority(topRightCell, table), topCell.rightBorder(), topCell.style().isInhRightBorder(), cellAreaPriority(topCell, table));
		else if (topRightCell.isValid())
			*topRight = collapseBorderAgainstTable(topRightCell.leftBorder(), topRightCell.style().isInhLeftBorder(), table->leftBorder());
		else if (topCell.isValid())
			*topRight = collapseBorderAgainstTable(topCell.rightBorder(), topCell.style().isInhRightBorder(), table->rightBorder());
		else
			*topRight = TableBorder();

		// Resolve right
		if (topRightCell.row() == bottomRightCell.row())
			*right = TableBorder();
		else if (topRightCell.isValid() && bottomRightCell.isValid())
			*right = collapseBordersBetweenCells(bottomRightCell.topBorder(), bottomRightCell.style().isInhTopBorder(), cellAreaPriority(bottomRightCell, table), topRightCell.bottomBorder(), topRightCell.style().isInhBottomBorder(), cellAreaPriority(topRightCell, table));
		else if (topRightCell.isValid())
			*right = collapseBorderAgainstTable(topRightCell.bottomBorder(), topRightCell.style().isInhBottomBorder(), table->bottomBorder());
		else if (bottomRightCell.isValid())
			*right = collapseBorderAgainstTable(bottomRightCell.topBorder(), bottomRightCell.style().isInhTopBorder(), table->topBorder());
		else
			*right = TableBorder();

		// Resolve bottom right
		if (bottomRightCell.column() == bottomCell.column())
			*bottomRight = TableBorder();
		else if (bottomRightCell.isValid() && bottomCell.isValid())
			*bottomRight = collapseBordersBetweenCells(bottomRightCell.leftBorder(), bottomRightCell.style().isInhLeftBorder(), cellAreaPriority(bottomRightCell, table), bottomCell.rightBorder(), bottomCell.style().isInhRightBorder(), cellAreaPriority(bottomCell, table));
		else if (bottomRightCell.isValid())
			*bottomRight = collapseBorderAgainstTable(bottomRightCell.leftBorder(), bottomRightCell.style().isInhLeftBorder(), table->leftBorder());
		else if (bottomCell.isValid())
			*bottomRight = collapseBorderAgainstTable(bottomCell.rightBorder(), bottomCell.style().isInhRightBorder(), table->rightBorder());
		else
			*bottomRight = TableBorder();
	}

	void resolveBordersVertical(const TableCell& topLeftCell, const TableCell& topRightCell, const TableCell& leftCell, const TableCell& rightCell, const TableCell& bottomLeftCell,
								const TableCell& bottomRightCell, TableBorder* topLeft, TableBorder* top, TableBorder* topRight, TableBorder* center, TableBorder* bottomLeft, TableBorder* bottom, TableBorder* bottomRight, PageItem_Table* table)
	{
		if (!leftCell.isValid() && !rightCell.isValid())
			return;

		// Resolve top left
		if (topLeftCell.row() == leftCell.row())
			*topLeft = TableBorder();
		else if (topLeftCell.isValid() && leftCell.isValid())
			*topLeft = collapseBordersBetweenCells(leftCell.topBorder(), leftCell.style().isInhTopBorder(), cellAreaPriority(leftCell, table), topLeftCell.bottomBorder(), topLeftCell.style().isInhBottomBorder(), cellAreaPriority(topLeftCell, table));
		else if (topLeftCell.isValid())
			*topLeft = collapseBorderAgainstTable(topLeftCell.bottomBorder(), topLeftCell.style().isInhBottomBorder(), table->bottomBorder());
		else if (leftCell.isValid())
			*topLeft = collapseBorderAgainstTable(leftCell.topBorder(), leftCell.style().isInhTopBorder(), table->topBorder());
		else
			*topLeft = TableBorder();

		// Resolve top
		if (topLeftCell.column() == topRightCell.column())
			*top = TableBorder();
		else if (topLeftCell.isValid() && topRightCell.isValid())
			*top = collapseBordersBetweenCells(topRightCell.leftBorder(), topRightCell.style().isInhLeftBorder(), cellAreaPriority(topRightCell, table), topLeftCell.rightBorder(), topLeftCell.style().isInhRightBorder(), cellAreaPriority(topLeftCell, table));
		else if (topLeftCell.isValid())
			*top = collapseBorderAgainstTable(topLeftCell.rightBorder(), topLeftCell.style().isInhRightBorder(), table->rightBorder());
		else if (topRightCell.isValid())
			*top = collapseBorderAgainstTable(topRightCell.leftBorder(), topRightCell.style().isInhLeftBorder(), table->leftBorder());
		else
			*top = TableBorder();

		// Resolve top right
		if (topRightCell.row() == rightCell.row())
			*topRight = TableBorder();
		else if (topRightCell.isValid() && rightCell.isValid())
			*topRight = collapseBordersBetweenCells(rightCell.topBorder(), rightCell.style().isInhTopBorder(), cellAreaPriority(rightCell, table), topRightCell.bottomBorder(), topRightCell.style().isInhBottomBorder(), cellAreaPriority(topRightCell, table));
		else if (topRightCell.isValid())
			*topRight = collapseBorderAgainstTable(topRightCell.bottomBorder(), topRightCell.style().isInhBottomBorder(), table->bottomBorder());
		else if (rightCell.isValid())
			*topRight = collapseBorderAgainstTable(rightCell.topBorder(), rightCell.style().isInhTopBorder(), table->topBorder());
		else
			*topRight = TableBorder();

		// Resolve center
		if (leftCell.column() == rightCell.column())
			*center = TableBorder();
		else if (leftCell.isValid() && rightCell.isValid())
			*center = collapseBordersBetweenCells(rightCell.leftBorder(), rightCell.style().isInhLeftBorder(), cellAreaPriority(rightCell, table), leftCell.rightBorder(), leftCell.style().isInhRightBorder(), cellAreaPriority(leftCell, table));
		else if (leftCell.isValid())
			*center = collapseBorderAgainstTable(leftCell.rightBorder(), leftCell.style().isInhRightBorder(), table->rightBorder());
		else if (rightCell.isValid())
			*center = collapseBorderAgainstTable(rightCell.leftBorder(), rightCell.style().isInhLeftBorder(), table->leftBorder());
		else
			*center = TableBorder();

		// Resolve bottom left
		if (bottomLeftCell.row() == leftCell.row())
			*bottomLeft = TableBorder();
		else if (bottomLeftCell.isValid() && leftCell.isValid())
			*bottomLeft = collapseBordersBetweenCells(bottomLeftCell.topBorder(), bottomLeftCell.style().isInhTopBorder(), cellAreaPriority(bottomLeftCell, table), leftCell.bottomBorder(), leftCell.style().isInhBottomBorder(), cellAreaPriority(leftCell, table));
		else if (bottomLeftCell.isValid())
			*bottomLeft = collapseBorderAgainstTable(bottomLeftCell.topBorder(), bottomLeftCell.style().isInhTopBorder(), table->topBorder());
		else if (leftCell.isValid())
			*bottomLeft = collapseBorderAgainstTable(leftCell.bottomBorder(), leftCell.style().isInhBottomBorder(), table->bottomBorder());
		else
			*bottomLeft = TableBorder();

		// Resolve bottom
		if (bottomLeftCell.column() == bottomRightCell.column())
			*bottom = TableBorder();
		else if (bottomLeftCell.isValid() && bottomRightCell.isValid())
			*bottom = collapseBordersBetweenCells(bottomRightCell.leftBorder(), bottomRightCell.style().isInhLeftBorder(), cellAreaPriority(bottomRightCell, table), bottomLeftCell.rightBorder(), bottomLeftCell.style().isInhRightBorder(), cellAreaPriority(bottomLeftCell, table));
		else if (bottomLeftCell.isValid())
			*bottom = collapseBorderAgainstTable(bottomLeftCell.rightBorder(), bottomLeftCell.style().isInhRightBorder(), table->rightBorder());
		else if (bottomRightCell.isValid())
			*bottom = collapseBorderAgainstTable(bottomRightCell.leftBorder(), bottomRightCell.style().isInhLeftBorder(), table->leftBorder());
		else
			*bottom = TableBorder();

		// Resolve bottom right
		if (bottomRightCell.row() == rightCell.row())
			*bottomRight = TableBorder();
		else if (bottomRightCell.isValid() && rightCell.isValid())
			*bottomRight = collapseBordersBetweenCells(bottomRightCell.topBorder(), bottomRightCell.style().isInhTopBorder(), cellAreaPriority(bottomRightCell, table), rightCell.bottomBorder(), rightCell.style().isInhBottomBorder(), cellAreaPriority(rightCell, table));
		else if (bottomRightCell.isValid())
			*bottomRight = collapseBorderAgainstTable(bottomRightCell.topBorder(), bottomRightCell.style().isInhTopBorder(), table->topBorder());
		else if (rightCell.isValid())
			*bottomRight = collapseBorderAgainstTable(rightCell.bottomBorder(), rightCell.style().isInhBottomBorder(), table->bottomBorder());
		else
			*bottomRight = TableBorder();
	}

	TableBorder collapseBorders(const TableBorder& firstBorder, const TableBorder& secondBorder)
	{
		// A border is "visible" if it has at least one line that will actually paint.
		// Borders that are null, zero-width, or colored "None" are treated as absent
		// so they don't override visible borders during collapse.
		const bool firstVisible = firstBorder.isVisible();
		const bool secondVisible = secondBorder.isVisible();

		if (!firstVisible && !secondVisible)
		{
			// Both borders are invisible, so return a null border.
			return TableBorder();
		}
		if (!firstVisible)
		{
			// First border is invisible, so return second border.
			return secondBorder;
		}
		if (!secondVisible)
		{
			// Second border is invisible, so return first border.
			return firstBorder;
		}

		// Both borders are visible.
		if (firstBorder.width() > secondBorder.width())
		{
			// First border is wider than second border, so return first border.
			return firstBorder;
		}
		if (firstBorder.width() < secondBorder.width())
		{
			// Second border is wider than first border, so return second border.
			return secondBorder;
		}

		// Borders have equal width.
		if (firstBorder.borderLines().size() > secondBorder.borderLines().size())
		{
			// First border has more border lines than second border, so return first border.
			return firstBorder;
		}
		if (firstBorder.borderLines().size() < secondBorder.borderLines().size())
		{
			// Second border has more border lines than first border, so return second border.
			return secondBorder;
		}

		// Borders are indistinguishable; return first border for deterministic results
		// regardless of argument order.
		return firstBorder;
	}

	void joinVertical(const TableBorder& border, const TableBorder& topLeft, const TableBorder& top,
					 const TableBorder& topRight, const TableBorder& bottomLeft, const TableBorder& bottom,
					 const TableBorder& bottomRight, QPointF* start, QPointF* end, QPointF* startOffsetFactors,
					 QPointF* endOffsetFactors)
	{
		Q_ASSERT(start);
		Q_ASSERT(end);
		Q_ASSERT(startOffsetFactors);
		Q_ASSERT(endOffsetFactors);

		// Reset offset coefficients.
		startOffsetFactors->setX(0.0);
		startOffsetFactors->setY(0.0);
		endOffsetFactors->setX(0.0);
		endOffsetFactors->setY(0.0);

		/*
	 * The numbered cases in the code below refers to the 45 possible join cases illustrated
	 * in the picture at http://wiki.scribus.net/canvas/File:Table_border_join_cases.png
	 */

		/*
	 * Adjust start point(s). Possible cases are 1-20, 26-39.
	 */
		if (border.joinsWith(topLeft))
		{
			if (border.joinsWith(topRight))
			{
				if (!border.joinsWith(top))
				{
					// Cases: 8, 19.
					startOffsetFactors->setY(-0.5);
				}
			}
			else if (!border.joinsWith(top))
			{
				if (top.joinsWith(topRight))
				{
					if (border.width() < top.width())
					{
						// Cases: 15A.
						start->setY(start->y() + 0.5 * top.width());
					}
					else
					{
						// Cases: 15B.
						startOffsetFactors->setY(-0.5);
					}
				}
				else
				{
					// Cases: 5, 17, 27, 38.
					startOffsetFactors->setY(-0.5);
				}
			}
		}
		else if (border.joinsWith(topRight))
		{
			if (!border.joinsWith(top))
			{
				if (top.joinsWith(topLeft))
				{
					if (border.width() < top.width())
					{
						// Cases: 14A.
						start->setY(start->y() + 0.5 * top.width());
					}
					else
					{
						// Cases: 14B.
						startOffsetFactors->setY(-0.5);
					}
				}
				else
				{
					// Cases: 4, 18, 32, 36.
					startOffsetFactors->setY(-0.5);
				}
			}
		}
		else if (border.joinsWith(top))
		{
			if (topLeft.joinsWith(topRight))
			{
				// Cases: 11.
				start->setY(start->y() + 0.5 * topLeft.width());
			}
		}
		else
		{
			// Cases: 1, 2, 3, 6, 12, 16, 20, 26, 28, 31, 33, 37, 39.
			start->setY(start->y() + 0.5 * qMax(topLeft.width(), topRight.width()));
		}
		// Cases: 7, 9, 10, 13, 29, 30, 34, 35 - No adjustment to start point(s) needed.

		/*
	 * Adjust end point(s). Possible cases are 1-15, 21-35, 40-43.
	 */
		if (border.joinsWith(bottomLeft))
		{
			if (border.joinsWith(bottomRight))
			{
				if (!border.joinsWith(bottom))
				{
					// Cases: 6, 24.
					endOffsetFactors->setY(0.5);
				}
			}
			else if (!border.joinsWith(bottom))
			{
				if (bottom.joinsWith(bottomRight))
				{
					if (bottom.width() < border.width())
					{
						// Cases: 14A.
						endOffsetFactors->setY(0.5);
					}
					else
					{
						// Cases: 14B.
						end->setY(end->y() - 0.5 * bottom.width());
					}
				}
				else
				{
					// Cases: 2, 22, 28, 42.
					endOffsetFactors->setY(0.5);
				}
			}
		}
		else if (border.joinsWith(bottomRight))
		{
			if (!border.joinsWith(bottom))
			{
				if (bottom.joinsWith(bottomLeft))
				{
					if (bottom.width() < border.width())
					{
						// Cases: 15A.
						endOffsetFactors->setY(0.5);
					}
					else
					{
						// Cases: 15B.
						end->setY(end->y() - 0.5 * bottom.width());
					}
				}
				else
				{
					// Cases: 3, 23, 33, 40.
					endOffsetFactors->setY(0.5);
				}
			}
		}
		else if (border.joinsWith(bottom))
		{
			if (bottomLeft.joinsWith(bottomRight))
			{
				// Cases: 11.
				end->setY(end->y() - 0.5 * bottomLeft.width());
			}
		}
		else
		{
			// Cases: 1, 4, 5, 8, 12, 21, 25, 26, 27, 31, 32, 41, 43.
			end->setY(end->y() - 0.5 * qMax(bottomLeft.width(), bottomRight.width()));
		}
		// Cases: 7, 9, 10, 13, 29, 30, 34, 35 - No adjustment to end point(s) needed.
	}

	void joinHorizontal(const TableBorder& border, const TableBorder& topLeft, const TableBorder& left,
						const TableBorder& bottomLeft, const TableBorder& topRight, const TableBorder& right,
						const TableBorder& bottomRight, QPointF* start, QPointF* end, QPointF* startOffsetFactors,
						QPointF* endOffsetFactors)
	{
		Q_ASSERT(start);
		Q_ASSERT(end);
		Q_ASSERT(startOffsetFactors);
		Q_ASSERT(endOffsetFactors);

		// Reset offset coefficients.
		startOffsetFactors->setX(0.0);
		startOffsetFactors->setY(0.0);
		endOffsetFactors->setX(0.0);
		endOffsetFactors->setY(0.0);

		/*
	 * The numbered cases in the code below refers to the 45 possible join cases illustrated
	 * in the picture at http://wiki.scribus.net/canvas/File:Table_border_join_cases.png
	 */

		/*
	 * Adjust start point(s). Possible cases are 1-25, 31-37, 40-41.
	 */
		if (border.joinsWith(bottomLeft))
		{
			if (border.joinsWith(topLeft))
			{
				if (border.joinsWith(left))
				{
					// Cases: 10.
					startOffsetFactors->setX(0.5);
				}
				else
				{
					// Cases: 7, 34.
					startOffsetFactors->setX(0.5);
				}
			}
			else
			{
				if (border.joinsWith(left))
				{
					// Cases: 8, 19.
					startOffsetFactors->setX(0.5);
				}
				else if (left.joinsWith(topLeft))
				{
					if (border.width() < left.width())
					{
						// Cases: 14A.
						start->setX(start->x() + 0.5 * left.width());
					}
					else
					{
						// Cases: 14B.
						startOffsetFactors->setX(0.5);
					}
				}
				else
				{
					// Cases: 4, 18, 32, 36.
					startOffsetFactors->setX(0.5);
				}
			}
		}
		else if (border.joinsWith(topLeft))
		{
			if (border.joinsWith(left))
			{
				// Cases: 6, 24.
				startOffsetFactors->setX(0.5);
			}
			else
			{
				if (left.joinsWith(bottomLeft))
				{
					if (left.width() < border.width())
					{
						// Cases: 15A.
						startOffsetFactors->setX(0.5);
					}
					else
					{
						// Cases: 15B.
						start->setX(start->x() + 0.5 * left.width());
					}
				}
				else
				{
					// Cases: 3, 23, 33, 40.
					startOffsetFactors->setX(0.5);
				}
			}
		}
		else if (!border.joinsWith(left) &&
				 (topLeft.joinsWith(bottomLeft) || topLeft.joinsWith(left) || bottomLeft.joinsWith(left)))
		{
			// Cases: 2, 5, 9, 13, 17, 22, 35.
			start->setX(start->x() + 0.5 * qMax(topLeft.width(), bottomLeft.width()));
		}
		else if (left.isNull())
		{
			// Cases: 31, 37, 41.
			start->setX(start->x() - 0.5 * qMax(topLeft.width(), bottomLeft.width()));
		}
		// Cases: 1, 11, 12, 16, 20, 21, 25 - No adjustment to start point(s) needed.

		/*
	 * Adjust end point(s). Possible cases are 1-30, 38-39, 42-43.
	 */
		if (border.joinsWith(bottomRight))
		{
			if (border.joinsWith(topRight))
			{
				if (border.joinsWith(right))
				{
					// Cases: 10.
					endOffsetFactors->setX(-0.5);
				}
				else
				{
					// Cases: 9, 29.
					endOffsetFactors->setX(-0.5);
				}
			}
			else if (border.joinsWith(right))
			{
				// Cases: 6, 24.
				endOffsetFactors->setX(-0.5);
			}
			else
			{
				if (right.joinsWith(topRight))
				{
					if (border.width() < right.width())
					{
						// Cases: 15A.
						end->setX(end->x() - 0.5 * right.width());
					}
					else
					{
						// Cases: 15B.
						endOffsetFactors->setX(-0.5);
					}
				}
				else
				{
					// Cases: 5, 17, 27, 38.
					endOffsetFactors->setX(-0.5);
				}
			}
		}
		else if (border.joinsWith(topRight))
		{
			if (border.joinsWith(right))
			{
				// Cases: 6, 24.
				endOffsetFactors->setX(-0.5);
			}
			else
			{
				if (right.joinsWith(bottomRight))
				{
					if (right.width() < border.width())
					{
						// Cases: 14A.
						endOffsetFactors->setX(-0.5);
					}
					else
					{
						// Cases: 14B.
						end->setX(end->x() - 0.5 * right.width());
					}
				}
				else
				{
					// Cases: 2, 22, 28, 42.
					endOffsetFactors->setX(-0.5);
				}
			}
		}
		else if (!border.joinsWith(right) &&
				 (topRight.joinsWith(bottomRight) || topRight.joinsWith(right) || bottomRight.joinsWith(right)))
		{
			// Cases: 3, 4, 7, 13, 18, 23, 30.
			end->setX(end->x() - 0.5 * qMax(topRight.width(), bottomRight.width()));
		}
		else if (right.isNull())
		{
			// Cases: 26, 39, 43.
			end->setX(end->x() + 0.5 * qMax(topRight.width(), bottomRight.width()));
		}
		// Cases: 1, 11, 12, 16, 20, 21, 25 - No adjustment to end point(s) needed.
	}

} // namespace TableUtils
