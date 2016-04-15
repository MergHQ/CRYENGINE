// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef BehaviorTreeSerializationSupport_h
	#define BehaviorTreeSerializationSupport_h

	#include <CrySerialization/IArchive.h>

namespace BehaviorTree
{

	#ifdef USING_BEHAVIOR_TREE_SERIALIZATION

//! A simple helper class in which we store hints to the serialization code.
//! These hints can be used to control the layout/contents of the property tree in the editor.
struct NodeSerializationHints
{
	NodeSerializationHints()
		: showXmlLineNumbers(false)
		, allowAutomaticMigration(false)
	{
	}

	bool showXmlLineNumbers;
	bool allowAutomaticMigration;
};

static void HandleXmlLineNumberSerialization(Serialization::IArchive& archive, const uint32 xmlLineNumber)
{
	const NodeSerializationHints* hintsContext = archive.context<NodeSerializationHints>();
	IF_LIKELY(hintsContext != nullptr)
	{
		if (hintsContext->showXmlLineNumbers)
		{
			string xmlLineText;
			const bool isXmlLineKnown = (xmlLineNumber != 0u);   // A bit 'hacky' but XML lines start at 1, so this works when the document has not been saved to XML yet.
			if (isXmlLineKnown)
			{
				xmlLineText.Format("::: %u :::", xmlLineNumber);
			}
			else
			{
				xmlLineText = "::: ? :::";
			}

			archive(xmlLineText, "", "!^^");
		}
	}
}

	#endif

}

#endif // BehaviorTreeSerializationSupport_h
