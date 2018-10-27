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

namespace LeechCraft
{
namespace Util
{
	template<typename T>
	class BitFlags
	{
		static_assert (std::is_enum<T>::value, "The instantiating type should be a enumeration");

		using St_t = std::underlying_type_t<T>;
		St_t Storage_ = 0;
	public:
		BitFlags () = default;

		BitFlags (T t)
		: Storage_ { static_cast<St_t> (t) }
		{
		}

		explicit operator bool () const
		{
			return Storage_;
		}

		BitFlags& operator&= (BitFlags other)
		{
			Storage_ &= other.Storage_;
			return *this;
		}

		BitFlags& operator|= (BitFlags other)
		{
			Storage_ |= other.Storage_;
			return *this;
		}

		friend BitFlags operator& (BitFlags left, BitFlags right)
		{
			left &= right;
			return left;
		}

		friend BitFlags operator| (BitFlags left, BitFlags right)
		{
			left |= right;
			return left;
		}
	};
}
}

#define DECLARE_BIT_FLAGS(F) \
		inline LeechCraft::Util::BitFlags<F> operator& (F left, F right) \
		{ \
			return LeechCraft::Util::BitFlags<F> { left } & right; \
		} \
		inline LeechCraft::Util::BitFlags<F> operator| (F left, F right) \
		{ \
			return LeechCraft::Util::BitFlags<F> { left } | right; \
		}
