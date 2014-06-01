/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#ifndef PLUGINS_QROSP_WRAPPERS_ENTITYWRAPPER_H
#define PLUGINS_QROSP_WRAPPERS_ENTITYWRAPPER_H
#include <QObject>
#include <qross/core/wrapperinterface.h>
#include <interfaces/structures.h>

namespace LeechCraft
{
namespace Qrosp
{
	class EntityWrapper : public QObject, public Qross::WrapperInterface
	{
		Q_OBJECT

		mutable Entity E_;

		Q_PROPERTY (QVariant Entity READ GetEntity WRITE SetEntity)
		Q_PROPERTY (QString Location READ GetLocation WRITE SetLocation)
		Q_PROPERTY (QString Mime READ GetMime WRITE SetMime)
		Q_PROPERTY (TaskParameters Parameters READ GetParameters WRITE SetParameters)
		Q_PROPERTY (QVariantMap Additional READ GetAdditional WRITE SetAdditional)
	public:
		EntityWrapper ();
		EntityWrapper (const EntityWrapper&);
		EntityWrapper (const Entity&);

		void* wrappedObject () const;
	public slots:
		LeechCraft::Entity Native () const;
		const QVariant& GetEntity () const;
		void SetEntity (const QVariant&);
		const QString& GetLocation () const;
		void SetLocation (const QString&);
		const QString& GetMime () const;
		void SetMime (const QString&);
		const TaskParameters& GetParameters () const;
		void SetParameters (const TaskParameters&);
		const QVariantMap& GetAdditional () const;
		void SetAdditional (const QVariantMap&);
	};

	QVariant EntityHandler (void*);
}
}

Q_DECLARE_METATYPE (LeechCraft::Qrosp::EntityWrapper);

#endif
