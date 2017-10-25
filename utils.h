#pragma once

#include "pch.h"

#ifdef _UTF16_STRINGS
#define N(x) utf16_to_utf8(x)
#define W(x) utf8_to_utf16(x)
#else
#define N(x) (x)
#define W(x) (x)
#endif

pplx::task<void> async_do_while(std::function<pplx::task<bool>(void)> func);

template <class T, class _Rep, class _Period>
pplx::task<T> task_after(T w, std::chrono::duration<_Rep, _Period> delay) {
    return pplx::create_task([=] {
        std::this_thread::sleep_for(delay);
        return w;
    });
}

template <class _Rep, class _Period>
pplx::task<void> task_delay(std::chrono::duration<_Rep, _Period> delay) {
    return pplx::create_task([=] {
        std::this_thread::sleep_for(delay);
    });
}

std::string utf16_to_utf8(const std::wstring &w);
std::wstring utf8_to_utf16(const std::string &w);


template<typename _Iter, typename _Func>
pplx::task<void> sequential_for(_Iter begin, _Iter end, _Func transform)
{
    typedef typename std::iterator_traits<_Iter>::value_type _Elem;
    typedef const typename std::iterator_traits<_Iter>::pointer _PElem;
    typedef const typename std::iterator_traits<_Iter>::reference _CRef;

    pplx::task<void> last_task = pplx::task_from_result();

    while (begin != end)
    {
        _CRef v = *begin;
        ++begin;

        last_task = last_task.then([&v, transform] {
            pplx::task<void> transformed = transform(v);
            return transformed;
        });

    }

    return last_task;
}


template<typename _Iter, typename _Func>
auto sequential_transform(_Iter begin, _Iter end, _Func transform)
-> decltype(pplx::task<std::vector<typename std::iterator_traits<_Iter>::value_type>>())
{
    typedef typename std::iterator_traits<_Iter>::value_type _Elem;
    typedef const typename std::iterator_traits<_Iter>::pointer _PElem;
    typedef const typename std::iterator_traits<_Iter>::reference _CRef;
    auto poutput = std::make_shared<std::vector<_Elem>>();

    pplx::task<void> last_task = pplx::task_from_result();

    while (begin != end)
    {
        _CRef v = *begin;
        ++begin;

        last_task = last_task.then([&v, poutput, transform] {

            pplx::task<_Elem> transformed = transform(v);

            return transformed.then([poutput](_Elem result) {
                poutput->emplace_back(result);
            });
        });

    }

    return last_task.then([=] {
        return std::forward<std::vector<_Elem>>(*poutput);
    });
}