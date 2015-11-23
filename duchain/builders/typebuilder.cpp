/*************************************************************************************
 *  Copyright (C) 2015 by Thomas Brix Larsen <brix@brix-verden.dk>                   *
 *  Copyright (C) 2014 by Pavel Petrushkov <onehundredof@gmail.com>                  *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "typebuilder.h"

#include <language/duchain/types/arraytype.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/integraltype.h>
#include <language/duchain/types/pointertype.h>
#include <language/duchain/types/structuretype.h>

#include "helper.h"

using namespace KDevelop;

namespace dlang
{

void TypeBuilder::visitTypeName(IType *node)
{
	if(!node)
	{
		injectType<AbstractType>(AbstractType::Ptr(new IntegralType(IntegralType::TypeNone)));
		return;
	}
	buildTypeName(node->getName(), nullptr);
	if(node->isArray())
	{
		KDevelop::ArrayType::Ptr array(new KDevelop::ArrayType());
		array->setElementType(lastType());
		array->setDimension(0);
		injectType(array);
	}
	if(node->isPointer())
	{
		KDevelop::PointerType::Ptr pointer(new KDevelop::PointerType());
		pointer->setBaseType(lastType());
		injectType(pointer);
	}
}

void TypeBuilder::buildTypeName(IIdentifier *typeName, IIdentifier *fullName)
{
	uint type = IntegralType::TypeNone;
	QualifiedIdentifier id = identifierForNode(typeName);
	QString name = id.toString();
	//Builtin types.
	if(name == "void")
		type = KDevelop::IntegralType::TypeVoid;
	else if(name == "ubyte")
		type = KDevelop::IntegralType::TypeSbyte;
	else if(name == "ushort")
		type = KDevelop::IntegralType::TypeShort;
	else if(name == "uint")
		type = KDevelop::IntegralType::TypeInt;
	else if(name == "ulong")
		type = KDevelop::IntegralType::TypeLong;
	else if(name == "byte")
		type = KDevelop::IntegralType::TypeByte;
	else if(name == "short")
		type = KDevelop::IntegralType::TypeShort;
	else if(name == "int")
		type = KDevelop::IntegralType::TypeInt;
	else if(name == "long")
		type = KDevelop::IntegralType::TypeLong;
	else if(name == "float")
		type = KDevelop::IntegralType::TypeFloat;
	else if(name == "double")
		type = KDevelop::IntegralType::TypeDouble;
	else if(name == "real")
		type = KDevelop::IntegralType::TypeDouble;
	else if(name == "char")
		type = KDevelop::IntegralType::TypeChar;
	else if(name == "wchar")
		type = KDevelop::IntegralType::TypeChar16_t;
	else if(name == "dchar")
		type = KDevelop::IntegralType::TypeChar32_t;
	else if(name == "bool")
		type = KDevelop::IntegralType::TypeBoolean;
	
	if(type == IntegralType::TypeNone)
	{
		QualifiedIdentifier id(identifierForNode(typeName));
		if(fullName)
			id.push(identifierForNode(fullName));
		DeclarationPointer decl = dlang::getTypeDeclaration(id, currentContext());
		if(decl)
		{
			DUChainReadLocker lock;
			StructureType *type = new StructureType();
			type->setDeclaration(decl.data());
			injectType<AbstractType>(AbstractType::Ptr(type));
			return;
		}
		DelayedType *unknown = new DelayedType();
		unknown->setIdentifier(IndexedTypeIdentifier(id));
		injectType<AbstractType>(AbstractType::Ptr(unknown));
		return;
	}
	if(type != IntegralType::TypeNone)
		injectType<AbstractType>(AbstractType::Ptr(new KDevelop::IntegralType(type)));
}

void TypeBuilder::visitParameter(IParameter *node)
{
	TypeBuilderBase::visitParameter(node);
	currentFunctionType->addArgument(lastType());
}

void TypeBuilder::visitFuncDeclaration(IFunctionDeclaration *node)
{
	DUChainWriteLocker lock;
	clearLastType();
	
	visitTypeName(node->getReturnType());
	
	FunctionType::Ptr functionType = FunctionType::Ptr(new FunctionType());
	currentFunctionType = functionType;
	
	if(lastType())
		functionType->setReturnType(lastType());
	
	openType(functionType);
	
	closeType();
}

void TypeBuilder::visitClassDeclaration(IClassDeclaration *node)
{
	openType<KDevelop::StructureType>(KDevelop::StructureType::Ptr(new KDevelop::StructureType));
	{
		DUChainWriteLocker lock;
		openContext(node, editorFindRange(node, 0), DUContext::ContextType::Class, node->getName());
	}
	TypeBuilderBase::visitClassDeclaration(node);
	{
		DUChainWriteLocker lock;
		//currentType<KDevelop::StructureType>()->setContext(currentContext());
		closeContext();
	}
	//currentType<KDevelop::StructureType>()->setPrettyName(node->getName()->getString());
	//currentType<KDevelop::StructureType>()->setStructureType();
	closeType();
}

void TypeBuilder::visitStructDeclaration(IStructDeclaration *node)
{
	openType<KDevelop::StructureType>(KDevelop::StructureType::Ptr(new KDevelop::StructureType));
	{
		DUChainWriteLocker lock;
		openContext(node, editorFindRange(node, 0), DUContext::ContextType::Class, node->getName());
	}
	TypeBuilderBase::visitStructDeclaration(node);
	{
		DUChainWriteLocker lock;
		//currentType<KDevelop::StructureType>()->setContext(currentContext());
		closeContext();
	}
	//currentType<KDevelop::StructureType>()->setPrettyName(node->getName()->getString());
	//currentType<KDevelop::StructureType>()->setStructureType();
	closeType();
}

}
