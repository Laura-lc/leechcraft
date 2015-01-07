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

#pragma once

#include <QUrl>

#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

#include "sllconfig.h"

namespace LeechCraft
{
namespace Util
{
	/** @brief Manipulates query part of an QUrl object.
	 *
	 * This class abstracts away differences between Qt4 and Qt5 QUrl and
	 * QUrlQuery handling, and it should be used in all new code instead
	 * of direct calls to Qt API.
	 *
	 * This class is used as follows:
	 * # An object of this class is constructed on a (named) QUrl object.
	 * # New URL query parameters are added by calling this object with a
	 *   pair of matching key and value.
	 * # Existing URL query parameters are removed via the -= operator.
	 * # The URL is updated on UrlOperator object destruction.
	 *
	 * Intended usage:
	 * \code{.cpp}
		QUrl someUrl { ... };
		UrlOperator { someUrl }
				("key1", "value1")
				("key2", "value2");
	   \endcode
	 *
	 * Here, an unnamed UrlOperator object is created that is valid only
	 * inside the corresponding expression, thus the changes to
	 * <code>someUrl</code> are visible immediately after executing that line.
	 *
	 * @note The changes are \em guaranteed to be applied on UrlOperator
	 * object destruction. Nevertheless, they may still be applied
	 * earlier on during calls to operator()() and operator-=().
	 */
	class UTIL_SLL_API UrlOperator
	{
		QUrl& Url_;

#if QT_VERSION >= 0x050000
		QUrlQuery Query_;
#endif
	public:
		/** @brief Constructs the object modifying the query of \em url.
		 *
		 * @param[in] url The URL to modify.
		 */
		UrlOperator (QUrl& url);

		/** @brief Flushes any pending changes to the QUrl query and
		 * destroys the UrlOperator.
		 *
		 * @sa Flush()
		 * @sa operator()()
		 */
		~UrlOperator ();

		/** @brief Flushes any pending changes to the QUrl query.
		 */
		void Flush ();

		/** @brief Adds a new \em key = \em value parameters pair.
		 *
		 * If the URL already contains this \em key, a new value is added
		 * in addition to the already existing one.
		 *
		 * The key/value pair is encoded before it is added to the query.
		 * The key and value are also encoded into UTF-8. Both \em key
		 * and \em value are URL-encoded as well. So, this function is
		 * analogous in effect to standard relevant Qt APIs.
		 *
		 * @param[in] key The query parameter key.
		 * @param[in] value The query parameter value.
		 * @return This UrlOperator object.
		 */
		UrlOperator& operator() (const QString& key, const QString& value);

		/** @brief Returns the first query parameter under the \em key.
		 *
		 * If no such parameters exist, this function does nothing.
		 *
		 * @param[in] key The query parameter key.
		 * @return This UrlOperator object.
		 */
		UrlOperator& operator-= (const QString& key);

		/** @brief Flushes any pending changes to the QUrl query.
		 */
		QUrl operator() ();
	};
}
}
