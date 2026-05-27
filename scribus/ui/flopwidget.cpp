/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "flopwidget.h"

#include "iconmanager.h"
#include "scribusapp.h"

FlopWidget::FlopWidget(QWidget* parent) : FormWidget(parent)
{

	flopRealHeight = new ScToolButton();
	flopRealHeight->setCheckable(true);
	flopRealHeight->setChecked(true);	
	flopFontAscent = new ScToolButton();
	flopFontAscent->setCheckable(true);
	flopLineSpacing = new ScToolButton();
	flopLineSpacing->setCheckable(true);
	flopBaselineGrid = new ScToolButton();
	flopBaselineGrid->setCheckable(true);

	flopGroup = new ScButtonGroup();
	flopGroup->setExclusive(true);
	flopGroup->addButton(flopFontAscent,  FontAscentID);
	flopGroup->addButton(flopRealHeight,  RealHeightID);
	flopGroup->addButton(flopLineSpacing, LineSpacingID);
	flopGroup->addButton(flopBaselineGrid, BaselineGridID);

	addWidget(flopFontAscent);
	addWidget(flopRealHeight);
	addWidget(flopLineSpacing);
	addWidget(flopBaselineGrid);

	// Modifiy layout spacing. Layout is created by addWidget()
	layout()->setSpacing(0);

	iconSetChange();
	languageChange();

	connect(ScQApp, SIGNAL(iconSetChanged()), this, SLOT(iconSetChange()));
}

void FlopWidget::iconSetChange()
{
	IconManager &im = IconManager::instance();

	flopRealHeight->setIcon(im.loadIcon("flop-max"));
	flopBaselineGrid->setIcon(im.loadIcon("flop-baseline"));
	flopLineSpacing->setIcon(im.loadIcon("flop-linespace"));
	flopFontAscent->setIcon(im.loadIcon("flop-font"));
}

void FlopWidget::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
		return;
	}
	QWidget::changeEvent(e);
}

void FlopWidget::languageChange()
{
	setText(tr("First Line &Offset"));
	flopFontAscent->setToolTip( tr("Set the height of the first line of text frame to use the full ascent of the font(s) in use"));
	flopRealHeight->setToolTip( tr("Set the height of the first line of the text frame to use the tallest height of the included characters"));
	flopLineSpacing->setToolTip( tr("Set the height of the first line of the text frame to the specified line height"));
	flopBaselineGrid->setToolTip( tr("Set the base line of the first line of the text frame to the base line grid"));
}
