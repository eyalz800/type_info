type_info
=========

Cool C++ Single-Header Type Info Implementation For Fun and Profit

Abstract
--------
It is not uncommon to encounter C++ projects with turned off RTTI.
In general, turning RTTI off is non-standard, however, sometimes reality makes us
give up RTTI knowingly.

In most cases, a choice to disable RTTI is made because it conflicts
with the main C++ idea of "You only pay for what you use.", since RTTI is added to every
polymorphic type, whether or not it is ever used.

Motivation
----------
Provide a simple solution using standard C++, implementing most of the
functionality given by the built in C++ RTTI implementation, for RTTI disabled projects,
in a way that generates extra information for a class on demand, per the user's request.

Contents
--------

* Enables dynamic casting using `dyn_cast<Destination>(Source)` similar to dynamic_cast.
* Generates unique id for every type using `type_id<Type>()`.
* Obtain the type id of the dynamic type of an object using `type_id(Expression)`.
* Sibling casts are supported.
* Casts from a virtual base to derived are supported.

Example
-------

```cpp
#include "type_info.h"

void foo()
{
    class a
    {
    public:
        using zpp_bases = zpp::make_zpp_bases<>;
        virtual zpp::dynamic_type zpp_dynamic_type() const { return *this; }
        virtual ~a() = default;

    private:
        int value = 0x11111111;
    };

    class b
    {
    public:
        using zpp_bases = zpp::make_zpp_bases<>;
        virtual zpp::dynamic_type zpp_dynamic_type() const { return *this; }
        virtual ~b() = default;

    private:
        int value = 0x22222222;
    };

    class c : public a, public b
    {
    public:
        using zpp_bases = zpp::make_zpp_bases<a, b>;
        virtual zpp::dynamic_type zpp_dynamic_type() const override { return *this; }
        virtual ~c() = default;

    private:
        int value = 0x33333333;
    };

    auto object = std::make_unique<c>();
    c * pc = object.get();
    a * pa = pc;
    b * pb = pc;

    auto dyn_a_from_b = zpp::dyn_cast<a *>(pb);
    auto dyn_b_from_a = zpp::dyn_cast<b *>(pa);
    auto dyn_c_from_a = zpp::dyn_cast<c *>(pa);
    auto dyn_c_from_b = zpp::dyn_cast<c *>(pb);

    auto dyn_void_from_a = zpp::dyn_cast<void *>(pa);
    auto dyn_void_from_b = zpp::dyn_cast<void *>(pb);
    auto dyn_void_from_c = zpp::dyn_cast<void *>(pc);

    std::cout << "a: " << pa << ", dyn_a_from_b: " << dyn_a_from_b << '\n';
    std::cout << "b: " << pb << ", dyn_b_from_a: " << dyn_b_from_a << '\n';
    std::cout << "c: " << pc << ", dyn_c_from_a: " << dyn_c_from_a << '\n';
    std::cout << "c: " << pc << ", dyn_c_from_b: " << dyn_c_from_b << '\n';

    std::cout << "a: " << pa << ", dyn_void_from_a: " << dyn_void_from_a << '\n';
    std::cout << "b: " << pb << ", dyn_void_from_b: " << dyn_void_from_b << '\n';
    std::cout << "c: " << pc << ", dyn_void_from_c: " << dyn_void_from_c << '\n';

    std::cout << "type_id<a>(): " << zpp::type_id<a>() << '\n';
    std::cout << "type_id<b>(): " << zpp::type_id<b>() << '\n';
    std::cout << "type_id<c>(): " << zpp::type_id<c>() << '\n';

    std::cout << "type_id(*pa): " << zpp::type_id(*pa) << '\n';
    std::cout << "type_id(*pb): " << zpp::type_id(*pb) << '\n';
    std::cout << "type_id(*pc): " << zpp::type_id(*pc) << '\n';
}
```

Disclaimer
----------
1. Performance of virtual base to derived and cross casts have a significant performance impact in comparison
to simple down casts. There is much room for optimization however, feel free to share your implementation.
2. Almost no tests were performed.
3. Inaccessible base classes do not work in the current implementation, however it is fairly easy to implement.
4. I did not implement dynamic cast for reference types, this is also left as a challenge to the reader.
5. The current implementation does not attempt to deal with the case where
the dynamic type of the object has two base class subobjects of the same type. The solution may be simple,
but involves substantial performance impact.

Final Words
-----------
I have implemented this utility mostly for fun, I did not measure whether it is better or worse than the
built in C++ RTTI.

I imagine some projects with turned off RTTI can use this on a small subset of their types
without having their entire project configured for RTTI.

Feel free to report issues or submit your own improvements via pull requests.

