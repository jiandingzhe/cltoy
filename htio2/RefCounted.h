#ifndef HTIO2_REF_COUNTED_H
#define	HTIO2_REF_COUNTED_H

#include <stdint.h>

#ifdef _MSC_VER
#define noexcept throw()
#endif

class TestFramework;

namespace htio2
{

class RefCounted
{
    friend class ::TestFramework;
public:
    RefCounted();
    RefCounted(const RefCounted& other);
    virtual ~RefCounted();

    RefCounted& operator=(const RefCounted& other);

    inline void ref() const;
    inline void unref() const;
    inline uint32_t get_ref_count() const;

private:
    mutable uint32_t counter;
};

inline RefCounted& RefCounted::operator=(const RefCounted& other)
{
    return *this;
}

inline void RefCounted::ref() const
{
    ++counter;
}

inline void RefCounted::unref() const
{
    if (--counter == 0)
        delete this;
}

inline uint32_t RefCounted::get_ref_count() const
{ return counter; }

inline void intrusive_ptr_add_ref(RefCounted* self)
{ self->ref(); }

inline void intrusive_ptr_release(RefCounted* self)
{ self->unref(); }

inline void smart_ref(const RefCounted* obj)
{ obj->ref(); }

inline void smart_unref(const RefCounted* obj)
{ obj->unref(); }

template <typename T>
class SmartPtr
{
    friend class RefCounted;
    friend class __tester__;
    template<typename T1, typename T2>
    friend bool operator == (const SmartPtr<T1>& a, const SmartPtr<T2>& b);
    template<typename T1, typename T2>
    friend bool operator < (const SmartPtr<T1>& a, const SmartPtr<T2>& b);
public:
    SmartPtr(): m_obj(0) {}

    SmartPtr(T* obj): m_obj(obj)
    {
        if (m_obj)
            smart_ref(m_obj);
    }

    SmartPtr(const SmartPtr& other): m_obj(other.m_obj)
    {
        if (m_obj)
            smart_ref(m_obj);
    }

    ~SmartPtr()
    {
        if (m_obj)
            smart_unref(m_obj);
    }

    SmartPtr& operator = (const SmartPtr& other)
    {
        if (m_obj)
            smart_unref(m_obj);

        m_obj = other.m_obj;

        if (m_obj)
            smart_ref(m_obj);

        return *this;
    }

    SmartPtr& operator = (T* other)
    {
        if (m_obj)
            smart_unref(m_obj);

        m_obj = other;

        if (m_obj)
            smart_ref(m_obj);

        return *this;
    }

    operator bool() const
    {
        return m_obj;
    }

    T* get() const
    {
        return m_obj;
    }

    T& operator*() const
    {
        return *m_obj;
    }

    T* operator->() const
    {
        return m_obj;
    }

protected:
    T* m_obj;
};

template<typename T1, typename T2>
bool operator == (const SmartPtr<T1>& a, const SmartPtr<T2>& b)
{
    return a.m_obj == b.m_obj;
}

template<typename T1, typename T2>
bool operator < (const SmartPtr<T1>& a, const SmartPtr<T2>& b)
{
    return a.m_obj < b.m_obj;
}

} // namespace htio2

#endif	/* HTIO2_REF_COUNTED_H */

