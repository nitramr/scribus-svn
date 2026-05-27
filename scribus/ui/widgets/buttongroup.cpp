#include "buttongroup.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QStyleHints>
#include <QStyleOption>
#include <QStylePainter>

ScButtonGroup::ScButtonGroup(QWidget *parent) : QButtonGroup(parent) {}

void ScButtonGroup::addButton(QAbstractButton *button, int id)
{
	button->installEventFilter(this);
	QButtonGroup::addButton(button, id);
	updateGroup();
}

void ScButtonGroup::removeButton(QAbstractButton *button)
{
	button->removeEventFilter(this);
	QButtonGroup::removeButton(button);
	updateGroup();
}

void ScButtonGroup::setAutoStyle(bool autoStyle)
{
	m_autoStyle = autoStyle;
	updateGroup();
}

void ScButtonGroup::updateGroup()
{
	QList<int> visButtons;

	// First gather all visible items
	for( int i = 0; i < buttons().size(); ++i )
	{
		if (buttons().at(i)->isVisible())
			visButtons.append(i);
	}

	int count = visButtons.size();

	// Apply style to visible buttons
	for( int i = 0; i < count; ++i )
	{
		ScToolButton *tb = qobject_cast<ScToolButton*>(buttons().at(visButtons.at(i)));
		if (!tb)
			return;

		if (!m_autoStyle || count == 1)
			tb->setPosition(ScToolButton::None);
		else
		{
			if (i == 0) // left
				tb->setPosition(ScToolButton::Left);
			else if (i == count - 1) // right
				tb->setPosition(ScToolButton::Right);
			else // center
				tb->setPosition(ScToolButton::Center);
		}
	}
}

bool ScButtonGroup::eventFilter(QObject *object, QEvent *event)
{
	if (object != nullptr && (event->type() == QEvent::Show || event->type() == QEvent::Hide))
		updateGroup();

	return QObject::eventFilter(object, event);
}
