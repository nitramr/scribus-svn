#ifndef SCBUTTONGROUP_H
#define SCBUTTONGROUP_H

#include <QButtonGroup>
#include <QProxyStyle>
#include <QToolButton>
#include <QWidget>

class ScToolButton : public QToolButton
{
	Q_OBJECT
public:
	enum Position
	{
		None = 0,
		Left,
		Center,
		Right
	};

	ScToolButton(QWidget* parent = nullptr) : QToolButton(parent) {};

	Position position() const { return m_position; };
	void setPosition(Position position) { m_position = position; };

private:
	Position m_position = Position::None;
};

class ScButtonGroup : public QButtonGroup
{
	Q_OBJECT

public:
	ScButtonGroup(QWidget* parent = nullptr);

	void addButton(QAbstractButton *button, int id = -1);
	void removeButton(QAbstractButton *button);

	bool autoStyle() { return m_autoStyle;}
	void setAutoStyle(bool autoStyle) ;

protected:
	void updateGroup();
	bool eventFilter(QObject *object, QEvent *event);
	bool m_autoStyle = true;
};

#endif // SCBUTTONGROUP_H
