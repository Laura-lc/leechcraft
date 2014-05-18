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

#include <type_traits>
#include <iterator>

namespace LeechCraft
{
namespace Util
{
	template<typename T1, typename T2, template<typename U> class Container, typename F>
	auto ZipWith (const Container<T1>& c1, const Container<T2>& c2, F f) -> Container<typename std::result_of<F (T1, T2)>::type>
	{
		Container<typename std::result_of<F (T1, T2)>::type> result;

		auto i1 = std::begin (c1), e1 = std::end (c1);
		auto i2 = std::begin (c2), e2 = std::end (c2);
		for ( ; i1 != e1 && i2 != e2; ++i1, ++i2)
			result.push_back (f (*i1, *i2));
		return result;
	}

	template<typename T, template<typename U> class Container, typename F>
	auto Map (const Container<T>& c, F f) -> Container<decltype (f (T ()))>
	{
		Container<decltype (f (T ()))> result;
		for (auto t : c)
			result.push_back (f (t));
		return result;
	}
}
}

