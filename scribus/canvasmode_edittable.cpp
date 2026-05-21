/*
Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QCursor>
#include <QDebug>
#include <QPainter>
#include <QPointF>
#include <QTimer>

#include "appmodes.h"
#include "canvas.h"
#include "canvasgesture_cellselect.h"
#include "canvasgesture_columnresize.h"
#include "canvasgesture_rowresize.h"
#include "canvasgesture_tableresize.h"
#include "canvasmode_edittable.h"
#include "fpoint.h"
#include "iconmanager.h"
#include "pageitem_table.h"
#include "scribus.h"
#include "scribusdoc.h"
#include "scribusview.h"
#include "selection.h"
#include "tablehandle.h"
#include "ui/contextmenu.h"
#include "undomanager.h"

CanvasMode_EditTable::CanvasMode_EditTable(ScribusView* view) : CanvasMode(view),
	m_canvasUpdateTimer(new QTimer(view)),
	m_selectRowCursor(IconManager::instance().loadCursor("cursor-select-row")),
	m_selectColumnCursor(IconManager::instance().loadCursor("cursor-select-column")),
	m_tableResizeGesture(new TableResize(this)),
	m_rowResizeGesture(new RowResize(this)),
	m_columnResizeGesture(new ColumnResize(this)),
	m_cellSelectGesture(new CellSelect(this)),
	m_ScMW(view->m_ScMW)
{
	connect(m_canvasUpdateTimer, SIGNAL(timeout()), this, SLOT(updateCanvas()));
}

CanvasMode_EditTable::~CanvasMode_EditTable()
{
	delete m_tableResizeGesture;
	delete m_rowResizeGesture;
	delete m_columnResizeGesture;
	delete m_cellSelectGesture;
}

void CanvasMode_EditTable::activate(bool fromGesture)
{
	CanvasMode::activate(fromGesture);

	m_canvas->resetRenderMode();

	PageItem *item = m_doc->m_Selection->itemAt(0);
	Q_ASSERT(item && item->isTable());
	m_table = item->asTable();

	if (fromGesture)
		m_view->setCursor(Qt::ArrowCursor);

	m_lastCursorPos = -1;
	m_blinkTime.start();
	m_canvasUpdateTimer->start(200);
	makeLongTextCursorBlink();

	m_view->m_ScMW->updateTableMenuActions();
}

void CanvasMode_EditTable::deactivate(bool forGesture)
{
	if (!forGesture)
		m_canvasUpdateTimer->stop();

	m_view->m_ScMW->updateTableMenuActions();
	CanvasMode::deactivate(forGesture);
}

void CanvasMode_EditTable::keyPressEvent(QKeyEvent* event)
{
	event->accept();

	// Escape: exit table edit mode.
	if (event->key() == Qt::Key_Escape)
	{
		PageItem_TextFrame* activeFrame = m_table->activeCell().textFrame();
		activeFrame->itemText.deselectAll();
		activeFrame->HasSel = false;
		m_view->requestMode(modeNormal);
		return;
	}

	const int key = event->key();
	const Qt::KeyboardModifiers mods = event->modifiers();
	// Arrow keys may carry KeypadModifier on macOS and some Linux layouts; mask it out before modifier comparisons.
	const Qt::KeyboardModifiers effMods = mods & ~Qt::KeypadModifier;
	const bool isArrow = (key == Qt::Key_Left  || key == Qt::Key_Right
						  || key == Qt::Key_Up    || key == Qt::Key_Down);

	// Tab / Shift+Tab: move to next/previous cell. Tab in the last cell
	// appends a new row, matching Word/Writer.
	if (key == Qt::Key_Tab && effMods == Qt::NoModifier)
	{
		const TableCell active = m_table->activeCell();
		const bool atLastCell =
				(active.row()    + active.rowSpan()    - 1 == m_table->rows()    - 1) &&
				(active.column() + active.columnSpan() - 1 == m_table->columns() - 1);
		if (atLastCell)
		{
			m_table->appendRows(1);
			updateCanvas();
		}
		navigateCells(Qt::Key_Right);
	}
	else if (key == Qt::Key_Backtab) // Qt delivers Shift+Tab as Backtab.
	{
		navigateCells(Qt::Key_Left);
	}
	// Plain arrow: caret movement within cell, escaping to next cell at edges.
	// Shift+arrow: extend text selection within cell, escaping to cell-range
	// selection at edges.
	// Plain arrow or Shift+arrow handling.
	else if (isArrow && (effMods == Qt::NoModifier || effMods == Qt::ShiftModifier))
	{
		const bool extending = (effMods == Qt::ShiftModifier);

		if (m_table->hasSelection())
		{
			// Already in cell-range selection mode.
			if (extending)
			{
				// Continue extending the cell range.
				extendCellSelection(key);
			}
			else
			{
				// Plain arrow collapses the cell selection and moves one cell.
				m_table->clearSelection();
				resetSelectionAnchor();
				navigateCells(key);
			}
		}
		else
		{
			// No cell selection: caret-level movement, escaping at edges.
			if (cursorAtCellBoundary(key))
			{
				if (extending)
					extendCellSelection(key);
				else
					navigateCells(key);
			}
			else
			{
				bool repeat;
				m_table->activeCell().textFrame()->handleModeEditKey(event, repeat);
			}
		}
	}
	// Everything else: pass to the active cell's text frame.
	else if (!m_table->hasSelection())
	{
		bool repeat;
		m_table->activeCell().textFrame()->handleModeEditKey(event, repeat);
	}

	makeLongTextCursorBlink();
	updateCanvas(true);
}

void CanvasMode_EditTable::mouseMoveEvent(QMouseEvent* event)
{
	event->accept();

	QPointF canvasPoint = m_canvas->globalToCanvas(event->globalPosition()).toQPointF();
	double threshold = m_doc->guidesPrefs().grabRadius / m_canvas->scale();
	TableHandle handle = m_table->hitTest(canvasPoint, threshold);

	if (event->buttons() & Qt::LeftButton)
	{
		// Mouse is being dragged with left button pressed.
		handleMouseDrag(event);
	}
	else
	{
		// Set an appropriate cursor.
		QCursor cursor(Qt::ArrowCursor);
		switch (handle.type())
		{
			case TableHandle::RowSelect:
				cursor = m_selectRowCursor;
				break;
			case TableHandle::RowResize:
				cursor = Qt::SizeVerCursor;
				break;
			case TableHandle::ColumnSelect:
				cursor = m_selectColumnCursor;
				break;
			case TableHandle::ColumnResize:
				cursor = Qt::SizeHorCursor;
				break;
			case TableHandle::TableResize:
				cursor = Qt::SizeFDiagCursor;
				break;
			case TableHandle::CellSelect:
				cursor = Qt::IBeamCursor;
				break;
			case TableHandle::None:
				break;
			default:
				qWarning("Unknown hit target");
				break;
		}
		m_view->setCursor(cursor);
	}
}

void CanvasMode_EditTable::mousePressEvent(QMouseEvent* event)
{
	if (UndoManager::undoEnabled())
	{
		SimpleState *ss = dynamic_cast<SimpleState*>(undoManager->getLastUndo());
		if (ss)
			ss->set("ETEA", QString(""));
		else
		{
			TransactionState *ts = dynamic_cast<TransactionState*>(undoManager->getLastUndo());
			if (ts)
				ss = dynamic_cast<SimpleState*>(ts->last());
			if (ss)
				ss->set("ETEA", QString(""));
		}
	}

	PageItem_TextFrame* activeFrame;

	event->accept();
	QPointF canvasPoint = m_canvas->globalToCanvas(event->globalPosition()).toQPointF();
	QPoint globalPos = event->globalPosition().toPoint();
	double threshold = m_doc->guidesPrefs().grabRadius / m_canvas->scale();
	TableHandle handle = m_table->hitTest(canvasPoint, threshold);
	TableCell cell;

	if (event->button() == Qt::LeftButton)
	{
		switch (handle.type())
		{
			case TableHandle::RowSelect:
				m_table->clearSelection();
				// Deselect text in active frame.
				activeFrame = m_table->activeCell().textFrame();
				activeFrame->itemText.deselectAll();
				activeFrame->HasSel = false;
				// Select the row indicated by the handle.
				m_table->moveTo(m_table->cellAt(handle.index(), 0));
				m_table->selectRow(handle.index());
				m_view->slotSetCurs(globalPos.x(), globalPos.y());
				m_lastCursorPos = -1;
				updateCanvas(true);
				break;
			case TableHandle::RowResize:
				// Start row resize gesture.
				m_rowResizeGesture->setup(m_table, handle.index());
				m_view->startGesture(m_rowResizeGesture);
				break;
			case TableHandle::ColumnSelect:
				m_table->clearSelection();
				activeFrame = m_table->activeCell().textFrame();
				activeFrame->itemText.deselectAll();
				activeFrame->HasSel = false;
				m_table->moveTo(m_table->cellAt(0, handle.index()));
				m_table->selectColumn(handle.index());
				m_view->slotSetCurs(globalPos.x(), globalPos.y());
				m_lastCursorPos = -1;
				updateCanvas(true);
				break;
			case TableHandle::ColumnResize:
				// Start column resize gesture.
				m_columnResizeGesture->setup(m_table, handle.index());
				m_view->startGesture(m_columnResizeGesture);
				break;
			case TableHandle::TableResize:
				// Start table resize gesture.
				m_tableResizeGesture->setup(m_table);
				m_view->startGesture(m_tableResizeGesture);
				break;
			case TableHandle::CellSelect:
				// Move to the pressed cell and position the text cursor.
				resetSelectionAnchor();
				m_table->clearSelection();
				m_table->moveTo(m_table->cellAt(canvasPoint));
				m_view->slotSetCurs(globalPos.x(), globalPos.y());
				m_lastCursorPos = m_table->activeCell().textFrame()->itemText.cursorPosition();
				m_view->m_ScMW->setTBvals(m_table->activeCell().textFrame());
				makeLongTextCursorBlink();
				updateCanvas(true);
				break;
			case TableHandle::None:
				// Deselect text in active frame.
				activeFrame = m_table->activeCell().textFrame();
				activeFrame->itemText.deselectAll();
				activeFrame->HasSel = false;
				// Deselect the table and go back to normal mode.
				m_view->deselectItems(true);
				m_view->requestMode(modeNormal);
				m_view->canvasMode()->mousePressEvent(event);
				break;
			default:
				qWarning("Unknown hit target");
				break;
		}
	}
	else if (event->button() == Qt::RightButton)
	{
		// Show the table popup menu.
		const FPoint mousePointDoc(m_canvas->globalToCanvas(event->globalPosition()));
		createContextMenu(m_table, mousePointDoc.x(), mousePointDoc.y());
	}
}

void CanvasMode_EditTable::mouseReleaseEvent(QMouseEvent* event)
{
	m_lastCursorPos = -1;
}

void CanvasMode_EditTable::mouseDoubleClickEvent(QMouseEvent* event)
{
	TableCell hitCell = m_table->cellAt(m_canvas->globalToCanvas(event->globalPosition()).toQPointF());
	if (!hitCell.isValid())
		return;

	event->accept();

	PageItem_TextFrame* textFrame = hitCell.textFrame();
	if (event->modifiers() & Qt::ControlModifier)
	{
		int start = 0, end = 0;

		if (event->modifiers() & Qt::ShiftModifier)
		{
			// Double click with Ctrl+Shift to select multiple paragraphs.
			uint oldParNr = textFrame->itemText.nrOfParagraph(m_lastCursorPos);
			uint newParNr = textFrame->itemText.nrOfParagraph();

			start = textFrame->itemText.startOfParagraph(qMin(oldParNr, newParNr));
			end = textFrame->itemText.endOfParagraph(qMax(oldParNr, newParNr));
		}
		else
		{
			// Double click with Ctrl to select a single paragraph.
			m_lastCursorPos = textFrame->itemText.cursorPosition();
			uint parNr = textFrame->itemText.nrOfParagraph(m_lastCursorPos);
			start = textFrame->itemText.startOfParagraph(parNr);
			end = textFrame->itemText.endOfParagraph(parNr);
		}
		textFrame->itemText.deselectAll();
		textFrame->itemText.extendSelection(start, end);
		textFrame->itemText.setCursorPosition(end);
	}
	else
	{
		// Regular double click selects a word.
		m_lastCursorPos = textFrame->itemText.cursorPosition();
		textFrame->itemText.selectWord(textFrame->itemText.cursorPosition());
	}

	updateCanvas(true);
}

void CanvasMode_EditTable::drawControls(QPainter* p)
{
	p->save();
	commonDrawControls(p, false);
	if (!m_table->hasSelection())
		drawTextCursor(p);
	p->restore();

	drawTableHandleHints(p);

	if (m_table->hasSelection())
		paintCellSelection(p);
}

void CanvasMode_EditTable::drawTableHandleHints(QPainter* p)
{
	if (!m_table || !m_canvas || !p)
		return;

	p->save();
	p->scale(m_canvas->scale(), m_canvas->scale());
	p->translate(-m_doc->minCanvasCoordinate.x(), -m_doc->minCanvasCoordinate.y());
	p->setTransform(m_table->getTransform(), true);
	p->setRenderHint(QPainter::Antialiasing);

	const double threshold = m_doc->guidesPrefs().grabRadius / m_canvas->scale();
	const QPointF offset = m_table->gridOffset();
	const double tableW = m_table->tableWidth();
	const double tableH = m_table->tableHeight();
	const double tickLength = qMax(threshold * 2.0, 8.0 / m_canvas->scale());
	const double tickWidth = qMax(2.0 / m_canvas->scale(), 1.0);

	QColor markerColor(50, 150, 220, 220);
	p->setPen(QPen(markerColor, tickWidth, Qt::SolidLine, Qt::FlatCap));

	// Tick marks along the top edge (one per column boundary, including outer).
	for (int c = 0; c <= m_table->columns(); ++c)
	{
		double colX = (c < m_table->columns()) ? m_table->columnPosition(c) : m_table->columnPosition(c - 1) + m_table->columnWidth(c - 1);
		p->drawLine(QPointF(offset.x() + colX, offset.y() - tickLength), QPointF(offset.x() + colX, offset.y()));
	}

	// Tick marks along the left edge (one per row boundary, including outer).
	for (int r = 0; r <= m_table->rows(); ++r)
	{
		double rowY = (r < m_table->rows()) ? m_table->rowPosition(r) : m_table->rowPosition(r - 1) + m_table->rowHeight(r - 1);
		p->drawLine(QPointF(offset.x() - tickLength, offset.y() + rowY), QPointF(offset.x(), offset.y() + rowY));
	}

	// Tick marks along the bottom edge.
	for (int c = 0; c <= m_table->columns(); ++c)
	{
		double colX = (c < m_table->columns()) ? m_table->columnPosition(c) : m_table->columnPosition(c - 1) + m_table->columnWidth(c - 1);
		p->drawLine(QPointF(offset.x() + colX, offset.y() + tableH), QPointF(offset.x() + colX, offset.y() + tableH + tickLength));
	}

	// Tick marks along the right edge.
	for (int r = 0; r <= m_table->rows(); ++r)
	{
		double rowY = (r < m_table->rows()) ? m_table->rowPosition(r) : m_table->rowPosition(r - 1) + m_table->rowHeight(r - 1);
		p->drawLine(QPointF(offset.x() + tableW, offset.y() + rowY), QPointF(offset.x() + tableW + tickLength, offset.y() + rowY));
	}

	p->restore();
}

void CanvasMode_EditTable::paintCellSelection(QPainter* p)
{
	if (!m_table || !m_canvas || !p)
			return;

	p->save();
	p->scale(m_canvas->scale(), m_canvas->scale());
	p->translate(-m_doc->minCanvasCoordinate.x(), -m_doc->minCanvasCoordinate.y());
	p->setTransform(m_table->getTransform(), true);
	p->setRenderHint(QPainter::Antialiasing);
	p->setPen(QPen(QColor(100, 200, 255), 3.0 / m_canvas->scale(), Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
	p->setBrush(QColor(100, 200, 255, 50));

	/*
	* The code below makes selectionPath a union of the cell rectangles of the selected cells.
	* Since the cell rectangles are adjacent, they must be expanded slightly (1.0) for the
	* uniting to work. This may not be the fastest way to compose the path of the selection,
	* but it makes for some very simple code. And the result looks good.
	*/

	const QPointF offset = m_table->gridOffset();
	QPainterPath selectionPath;

	for (const TableCell& cell : m_table->selectedCells())
	{
		QRectF cellRect = cell.boundingRect();
		cellRect.translate(offset);
		cellRect.adjust(-1.0, -1.0, 1.0, 1.0);
		QPainterPath cellPath;
		cellPath.addRect(cellRect);
		selectionPath = selectionPath.united(cellPath);
	}

	p->drawPath(selectionPath);
	p->restore();
}

void CanvasMode_EditTable::updateCanvas(bool forceRedraw)
{
	// Do not let update timer cancel forced redraw otherwise
	// we get refresh issues  when typing or selecting text
	if (!m_canvas->isForcedRedraw())
		m_canvas->setForcedRedraw(forceRedraw);
	// Enlarge the update region to include the selection visual, which
	// extends slightly outside cell boundaries.
	QRectF tableRect = m_table->getBoundingRect();
	tableRect.adjust(-5.0, -5.0, 5.0, 5.0);
	m_canvas->update(m_canvas->canvasToLocal(tableRect));
}

void CanvasMode_EditTable::handleMouseDrag(QMouseEvent* event)
{
	TableCell hitCell = m_table->cellAt(m_canvas->globalToCanvas(event->globalPosition()).toQPointF());
	TableCell activeCell = m_table->activeCell();
	PageItem_TextFrame* activeFrame = activeCell.textFrame();

	if ((!hitCell.isValid() || hitCell == activeCell) && m_lastCursorPos != -1)
	{
		// Select text in active cell text frame.
		activeFrame->itemText.deselectAll();
		activeFrame->HasSel = false;

		QPoint globalPos = event->globalPosition().toPoint();
		m_view->slotSetCurs(globalPos.x(), globalPos.y());

		PageItem_TextFrame* newActiveFrame = m_table->activeCell().textFrame();
		if (activeFrame == newActiveFrame)
		{
			const int selectionStart = qMin(activeFrame->itemText.cursorPosition(), m_lastCursorPos);
			const int selectionLength = qAbs(activeFrame->itemText.cursorPosition() - m_lastCursorPos);

			activeFrame->itemText.select(selectionStart, selectionLength);
			activeFrame->HasSel = (selectionLength > 0);
		}
		else
		{
			m_lastCursorPos = -1;
			m_cellSelectGesture->setup(m_table, activeCell);
			m_view->startGesture(m_cellSelectGesture);
		}
		m_view->m_ScMW->setTBvals(newActiveFrame);
	}
	else
	{
		/*
		 * Mouse moved into another cell, so deselect all text and start
		 * cell selection gesture.
		 */
		activeFrame->itemText.deselectAll();
		activeFrame->HasSel = false;

		m_cellSelectGesture->setup(m_table, activeCell);
		m_view->startGesture(m_cellSelectGesture);
	}

	updateCanvas(true);
}

void CanvasMode_EditTable::drawTextCursor(QPainter* p)
{
	if ((!m_longBlink && m_blinkTime.elapsed() > QApplication::cursorFlashTime() / 2)
			|| (m_longBlink && m_blinkTime.elapsed() > QApplication::cursorFlashTime()))
	{
		// Reset blink timer
		m_blinkTime.restart();
		m_longBlink = false;
		m_cursorVisible = !m_cursorVisible;
	}

	if (m_cursorVisible)
	{
		// Paint text cursor.
		p->save();
		p->setTransform(m_table->getTransform(), true);
		commonDrawTextCursor(p, m_table->activeCell().textFrame(), m_table->gridOffset());
		p->restore();
	}
}

void CanvasMode_EditTable::makeLongTextCursorBlink()
{
	m_cursorVisible = true;
	m_longBlink = true;
	m_blinkTime.restart();
}

void CanvasMode_EditTable::createContextMenu(PageItem *currItem, double mx, double my)
{
	ContextMenu* cmen = nullptr;
	m_view->setCursor(QCursor(Qt::ArrowCursor));
	m_view->setObjectUndoMode();
	if (currItem != nullptr)
		cmen = new ContextMenu(*(m_doc->m_Selection), m_ScMW, m_doc);
	else
		cmen = new ContextMenu(m_ScMW, m_doc, mx, my);
	if (cmen)
		cmen->exec(QCursor::pos());
	m_view->setGlobalUndoMode();
	delete cmen;
}

void CanvasMode_EditTable::resetSelectionAnchor()
{
	m_selectionAnchorRow = -1;
	m_selectionAnchorColumn = -1;
}

bool CanvasMode_EditTable::moveActiveCell(int key)
{
	switch (key)
	{
		case Qt::Key_Left:
			m_table->moveLeft();
			return true;
		case Qt::Key_Right:
			m_table->moveRight();
			return true;
		case Qt::Key_Up:
			m_table->moveUp();
			return true;
		case Qt::Key_Down:
			m_table->moveDown();
			return true;
	}
	return false;
}

void CanvasMode_EditTable::navigateCells(int key)
{
	const int beforeRow = m_table->activeCell().row();
	const int beforeCol = m_table->activeCell().column();

	if (!moveActiveCell(key))
		return;

	tryRowWrap(key, beforeRow, beforeCol);

	resetSelectionAnchor();
	m_table->clearSelection();

	PageItem_TextFrame* tf = m_table->activeCell().textFrame();
	tf->itemText.deselectAll();
	tf->HasSel = false;
	tf->itemText.setCursorPosition((key == Qt::Key_Left || key == Qt::Key_Up) ? tf->itemText.length() : 0);
}

void CanvasMode_EditTable::extendCellSelection(int key)
{
	if (!m_table->hasSelection())
	{
		m_selectionAnchorRow    = m_table->activeCell().row();
		m_selectionAnchorColumn = m_table->activeCell().column();
	}
	else if (m_selectionAnchorRow < 0)
	{
		// Selection exists (e.g. from mouse drag) but no keyboard anchor.
		// Use the corner of the bounding box opposite the active cell.
		int activeRow = m_table->activeCell().row();
		int activeCol = m_table->activeCell().column();
		int topRow = INT_MAX, bottomRow = INT_MIN;
		int leftCol = INT_MAX, rightCol = INT_MIN;
		for (const TableCell& cell : m_table->selectedCells())
		{
			topRow    = qMin(topRow,    cell.row());
			bottomRow = qMax(bottomRow, cell.row()    + cell.rowSpan()    - 1);
			leftCol   = qMin(leftCol,   cell.column());
			rightCol  = qMax(rightCol,  cell.column() + cell.columnSpan() - 1);
		}
		m_selectionAnchorRow    = (activeRow == bottomRow) ? topRow  : bottomRow;
		m_selectionAnchorColumn = (activeCol == rightCol)  ? leftCol : rightCol;
	}

	if (!moveActiveCell(key))
		return;

	// No row-wrap during selection extension: Word stops at row edges with
	// Shift+arrow, and a rectangular cell selection can't represent the
	// L-shaped region that row-wrap would imply.

	PageItem_TextFrame* tf = m_table->activeCell().textFrame();
	tf->itemText.deselectAll();
	tf->HasSel = false;

	m_table->clearSelection();
	m_table->selectCells(m_selectionAnchorRow, m_selectionAnchorColumn, m_table->activeCell().row(), m_table->activeCell().column());
}

bool CanvasMode_EditTable::cursorAtCellBoundary(int key) const
{
	PageItem_TextFrame* tf = m_table->activeCell().textFrame();
	const int len = tf->itemText.length();

	if (len == 0)
		return true;

	const int pos = tf->itemText.cursorPosition();
	bool result = false;

	switch (key)
	{
		case Qt::Key_Left:
			result = (pos <= 0);
			break;
		case Qt::Key_Right:
			result = (pos >= len);
			break;
		case Qt::Key_Up:
			result = tf->cursorOnFirstLine();
			break;
		case Qt::Key_Down:
			result = tf->cursorOnLastLine();
			break;
	}

	return result;
}

bool CanvasMode_EditTable::tryRowWrap(int key, int beforeRow, int beforeCol)
{
	// Returns true if the active cell didn't change after moveActiveCell()
	// and we successfully wrapped to a different row. Right at end of row
	// wraps to (row+1, 0); Left at start of row wraps to (row-1, lastCol).
	// Vertical motion does not wrap.
	if (m_table->activeCell().row() != beforeRow ||
		m_table->activeCell().column() != beforeCol)
		return false;

	if (key == Qt::Key_Right && beforeRow + 1 < m_table->rows())
	{
		m_table->moveTo(m_table->cellAt(beforeRow + 1, 0));
		return true;
	}
	if (key == Qt::Key_Left && beforeRow > 0)
	{
		m_table->moveTo(m_table->cellAt(beforeRow - 1, m_table->columns() - 1));
		return true;
	}
	return false;
}