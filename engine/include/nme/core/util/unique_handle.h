#pragma once

namespace nme {

template<class Traits>
class [[nodiscard]] UniqueHandle {
public:
    using handle_type = Traits::handle_type;

private:
    handle_type m_handle;

public:
    UniqueHandle() noexcept : m_handle(Traits::invalid()) {}
    explicit UniqueHandle(handle_type h) noexcept : m_handle(h) {}

    ~UniqueHandle() { reset(); }

    // move-only
    UniqueHandle(const UniqueHandle&)            = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : m_handle(other.release()) {}
    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) reset(other.release());  // close ours, adopt theirs
        return *this;
    }

    [[nodiscard]] handle_type get()   const noexcept { return m_handle; }
    [[nodiscard]] handle_type valid() const noexcept { return m_handle != Traits::invalid(); }
    explicit operator bool()          const noexcept { return valid(); }

    // Give up ownership without closing
    [[nodiscard]] handle_type release() noexcept {
        handle_type h = m_handle;
        m_handle = Traits::invalid();
        return h;
    }
    // Close the current handle
    void reset(handle_type h = Traits::invalid()) noexcept {
        if (valid()) Traits::close(m_handle);
        m_handle = h;
    }
};

}  // namespace nme
