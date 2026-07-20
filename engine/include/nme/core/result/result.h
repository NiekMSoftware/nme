#pragma once

#include <new>      // placement new
#include <utility>  // std::move, std::forward

#include "../../platform/debug/assert.h"
#include "nme/core/result/error.h"

namespace nme 
{
	template<class T>
	class [[nodiscard]] Result {
	private:
		union {
			T m_value;
			Error m_error;
		};
		b32 m_ok;

	public:
        Result(const T &value)  : m_value(value),  m_ok(true)  { }
        Result(T&& value)	     : m_value(value),  m_ok(true)  { }
        Result(const Error err) : m_error(err),    m_ok(false) {
			NME_ASSERT(NME_FAILED(err));    // a failure must carry a real error
		}

		~Result() { if (m_ok) m_value.~T(); }

		Result(const Result& o) : m_ok(o.m_ok) {
			if (m_ok) ::new (&m_value) T(o.m_value); else m_error = o.m_error;
		}
		Result(Result&& o) noexcept : m_ok(o.m_ok) {
			if (m_ok) ::new (&m_value) T(o.m_value); else m_error = o.m_error;
		}
		Result &operator=(const Result& o) {
			if (this != &o) { destroy(); m_ok = o.m_ok; 
				if (m_ok) ::new (&m_value) T(std::move(o.m_value)); else m_error = o.m_error; }
			return *this;
		}
		Result &operator=(Result&& o) noexcept {
			if (this != &o) { destroy(); m_ok = o.m_ok;
				if (m_ok) ::new (&m_value) T(std::move(o.m_value)); else m_error = o.m_error; }
			return *this;
		}

		[[nodiscard]] bool ok()	    const noexcept   { return m_ok; }
		explicit operator bool()    const noexcept   { return m_ok; }
		[[nodiscard]] Error error() const noexcept { return m_ok ? Error::None : m_error; }

		T&       value()&		{ NME_ASSERT(m_ok); return m_value; }
		const T& value() const&	{ NME_ASSERT(m_ok); return m_value; }
		T&&		 value()&&		{ NME_ASSERT(m_ok); return m_value; }

		template<class U>
		T value_or(U&& fallback) const& {
			return m_ok ? m_value : static_cast<T>(std::forward<U>(fallback));
		}

	private:
		void destroy() noexcept { if (m_ok) m_value.~T(); }
	};

}  // namespace nme
