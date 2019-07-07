// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>
#include <CrySystem/ICryPluginManager.h>
#include <CrySystem/IProjectManager.h>
#include "CSharpOutputModel.h"

#include "QAdvancedTreeView.h"
#include "ProxyModels/ItemModelAttribute.h"

#include <QVariant>
#include <QFilteringPanel.h>
#include <QAbstractItemModel>
#include "CryMono/IMonoRuntime.h"

namespace OutputModelAttributes
{
CItemModelAttributeEnumFunc s_SeverityAttribute("Severity", &CCSharpOutputModel::GetSeverityList);
CItemModelAttribute s_CodeAttribute("Code", &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true);
CItemModelAttribute s_DescriptionAttribute("Description", &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true);
CItemModelAttribute s_FileAttribute("File", &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true);
CItemModelAttribute s_LineAttribute("Line", &Attributes::s_intAttributeType, CItemModelAttribute::Visible, true);
}

QStringList CCSharpOutputModel::GetSeverityList()
{
	QStringList list;
	list.append(tr("Warning"));
	list.append(tr("Error"));
	return list;
}

CCSharpOutputModel::CCSharpOutputModel(QObject* pParent)
	: QAbstractItemModel(pParent)
{

}

void CCSharpOutputModel::ResetModel()
{
	beginResetModel();
	endResetModel();
}

QVariant CCSharpOutputModel::data(const QModelIndex& index, int role) const
{
	const Cry::IProjectManager* pProjectManager = gEnv->pSystem->GetIProjectManager();
	stack_string assetsDirectoryLower = stack_string(pProjectManager->GetCurrentAssetDirectoryAbsolute());
	assetsDirectoryLower.append("/");
	assetsDirectoryLower.MakeLower();
	const SCSharpCompilerError* pCompileError = gEnv->pMonoRuntime->GetCompileErrorAt(index.row());
	if (pCompileError != nullptr)
	{
		switch (role)
		{
		case Qt::DisplayRole:
			switch (index.column())
			{
			case eColumn_ErrorType:
				{
					switch (pCompileError->m_errorSeverity)
					{
					case SCSharpCompilerError::eESeverity_Warning:
						return "Warning";
					case SCSharpCompilerError::eESeverity_Error:
						return "Error";
					default:
						CRY_ASSERT_MESSAGE(false, "Unknown severity.");
					}
				}
			case eColumn_ErrorNumber:
				return pCompileError->m_errorNumber.c_str();
			case eColumn_ErrorText:
				return pCompileError->m_errorText.c_str();
			case eColumn_FileName:
				{
					stack_string filename = pCompileError->m_fileName;
					size_t splitterIndex = filename.rfind('/');
					if (splitterIndex != -1)
					{
						return QVariant(filename.substr(splitterIndex + 1, filename.length() - 1).c_str());
					}
				}
			case eColumn_Line:
				return pCompileError->m_line;
			default:
				break;
			}
			break;
		case Qt::ToolTipRole:
			switch (index.column())
			{
			case eColumn_FileName:
				return pCompileError->m_fileName.c_str();
			default:
				break;
			}
			break;
		case Qt::DecorationRole:
			if (index.column() == eColumn_ErrorType)
			{
				switch (pCompileError->m_errorSeverity)
				{
				case SCSharpCompilerError::eESeverity_Warning:
					return CryIcon("icons:Dialogs/dialog-warning.ico");
				case SCSharpCompilerError::eESeverity_Error:
					return CryIcon("icons:Dialogs/dialog-error.ico");
				default:
					CRY_ASSERT_MESSAGE(false, "Unknown severity.");
				}
			}
			break;
		}
	}

	return QVariant();
}

QVariant CCSharpOutputModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if ((orientation == Qt::Horizontal) && (section < CCSharpOutputModel::eColumn_COUNT))
	{
		const CItemModelAttribute* pAttribute = GetColumnAttribute(section);
		switch (role)
		{
		case Qt::DisplayRole:
			return pAttribute->GetName();
		case Attributes::s_getAttributeRole:
			return QVariant::fromValue(const_cast<CItemModelAttribute*>(pAttribute));
		}
	}
	CRY_ASSERT_MESSAGE(section < eColumn_COUNT, "Parameter 'section' must be within attribute range.");

	return QVariant();
}

int CCSharpOutputModel::rowCount(const QModelIndex& parent) const
{
	return !parent.isValid() ? gEnv->pMonoRuntime->GetCompileErrorCount() : 0;
}

const CItemModelAttribute* CCSharpOutputModel::GetColumnAttribute(int column) const
{
	switch (column)
	{
	case CCSharpOutputModel::eColumn_ErrorType:
		return &OutputModelAttributes::s_SeverityAttribute;
	case CCSharpOutputModel::eColumn_ErrorNumber:
		return &OutputModelAttributes::s_CodeAttribute;
	case CCSharpOutputModel::eColumn_ErrorText:
		return &OutputModelAttributes::s_DescriptionAttribute;
	case CCSharpOutputModel::eColumn_FileName:
		return &OutputModelAttributes::s_FileAttribute;
	case CCSharpOutputModel::eColumn_Line:
		return &OutputModelAttributes::s_LineAttribute;
	default:
		return nullptr;
	}
}