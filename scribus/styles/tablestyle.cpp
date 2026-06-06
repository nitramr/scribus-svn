/*
 Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

 For general Scribus (>=1.3.2) copyright and licensing information please refer
 to the COPYING file provided with the program. Following this notice may exist
 a copyright and/or license notice that predates the release of Scribus 1.3.2
 for which a new license (GPL+exception) is in place.
 */

#include "commonstrings.h"
#include "desaxe/saxiohelper.h"
#include "util_math.h"
#include "tableborder.h"
#include "tablestyle.h"

QString TableStyle::displayName() const
{
	if (isDefaultStyle())
		return CommonStrings::trDefaultTableStyle;
	if (hasName() || !hasParent() || !m_context)
		return name();
	return parentStyle()->displayName() + "+";
}

bool TableStyle::equiv(const BaseStyle& other) const
{
	other.validate();
	const TableStyle *p_other = dynamic_cast<const TableStyle*> (&other);
	if (!p_other)
		return false;
	if (parent() != p_other->parent())
		return false;
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	if (inh_##attr_NAME != p_other->inh_##attr_NAME) \
		return false; \
	if (!inh_##attr_NAME && !isequiv(m_##attr_NAME, p_other->m_##attr_NAME)) \
		return false;
#include "tablestyle.attrdefs.cxx"
#undef ATTRDEF

	// Conditional style map: same set of areas, each style equivalent.
	// (Configuration scalars are now attrdef-managed and compared above.)
	if (m_conditionalStyles.size() != p_other->m_conditionalStyles.size())
		return false;
	for (auto it = m_conditionalStyles.constBegin(); it != m_conditionalStyles.constEnd(); ++it)
	{
		auto otherIt = p_other->m_conditionalStyles.constFind(it.key());
		if (otherIt == p_other->m_conditionalStyles.constEnd())
			return false;
		if (!it.value().equiv(otherIt.value()))
			return false;
	}

	return true;
}

void TableStyle::erase()
{
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	if (!inh_##attr_NAME) \
		reset##attr_NAME();
#include "tablestyle.attrdefs.cxx"
#undef ATTRDEF
	// Conditional map is owned, not attrdef-managed, so clear it explicitly.
	m_conditionalStyles.clear();
	//updateFeatures(); TODO: Investigate this.
}

void TableStyle::update(const StyleContext* context)
{
	BaseStyle::update(context);
	const TableStyle* parent = dynamic_cast<const TableStyle*>(parentStyle());
	if (parent) {
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
		if (inh_##attr_NAME) \
			m_##attr_NAME = parent->attr_GETTER();
#include "tablestyle.attrdefs.cxx"
#undef ATTRDEF
	}
	//updateFeatures(); TODO: Investigate this.
}

void TableStyle::getNamedResources(ResourceCollection& lists) const
{
	for (const BaseStyle* style = parentStyle(); style != nullptr; style = style->parentStyle())
		lists.collectCellStyle(style->name());
	lists.collectColor(fillColor());
	// TODO: Collect colors of borders.

	// Conditional styles may reference fill (and border) colors.
	for (auto it = m_conditionalStyles.constBegin(); it != m_conditionalStyles.constEnd(); ++it)
		it.value().getNamedResources(lists);
}

void TableStyle::replaceNamedResources(ResourceCollection& newNames)
{
	QMap<QString, QString>::ConstIterator it;

	if (!isInhFillColor() && (it = newNames.colors().find(fillColor())) != newNames.colors().end())
		setFillColor(it.value());

	// TODO: Do we need to do something else? E.g. CharStyle calls its updateFeatures().
}

bool TableStyle::hasConditionalStyleResolved(TableArea area) const
{
	for (const TableStyle* s = this; s != nullptr;
		 s = dynamic_cast<const TableStyle*>(s->parentStyle()))
	{
		if (s->m_conditionalStyles.contains(area))
			return true;
	}
	return false;
}

CellStyle TableStyle::conditionalStyleResolved(TableArea area) const
{
	for (const TableStyle* s = this; s != nullptr;
		 s = dynamic_cast<const TableStyle*>(s->parentStyle()))
	{
		auto it = s->m_conditionalStyles.constFind(area);
		if (it != s->m_conditionalStyles.constEnd())
			return it.value();
	}
	return CellStyle();
}

QList<TableArea> TableStyle::conditionalAreasResolved() const
{
	QList<TableArea> areas;
	for (const TableStyle* s = this; s != nullptr;
		 s = dynamic_cast<const TableStyle*>(s->parentStyle()))
	{
		for (auto it = s->m_conditionalStyles.constBegin();
			 it != s->m_conditionalStyles.constEnd(); ++it)
		{
			if (!areas.contains(it.key()))
				areas.append(it.key());
		}
	}
	return areas;
}

void TableStyle::saxx(SaxHandler& handler, const Xml_string& elemtag) const
{
	/*
	Xml_attr att;
	Style::saxxAttributes(att);
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	if (!inh_##attr_NAME) \
		att.insert(# attr_NAME, toXMLString(m_##attr_NAME));
#include "tablestyle.attrdefs.cxx"
#undef ATTRDEF
	if (!name().isEmpty())
		att["id"] = mkXMLName(elemtag + name());
	handler.begin(elemtag, att);
//	if (hasParent() && parentStyle())
//		parentStyle()->saxx(handler);
	handler.end(elemtag);
	*/
}

using namespace desaxe;

const Xml_string TableStyle::saxxDefaultElem("tablestyle");

void TableStyle::desaxeRules(const Xml_string& prefixPattern, Digester& ruleset, Xml_string elemtag)
{
	/*
	Xml_string stylePrefix(Digester::concat(prefixPattern, elemtag));
	ruleset.addRule(stylePrefix, Factory<TableStyle>());
	ruleset.addRule(stylePrefix, IdRef<TableStyle>());
	Style::desaxeRules<TableStyle>(prefixPattern, ruleset, elemtag);
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	ruleset.addRule(stylePrefix, SetAttributeWithConversion<TableStyle, attr_TYPE> ( & TableStyle::set##attr_NAME,  # attr_NAME, &parse<attr_TYPE> ));
#include "tablestyle.attrdefs.cxx"
#undef ATTRDEF
*/
}