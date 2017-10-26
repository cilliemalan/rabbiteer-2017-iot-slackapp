#include "pch.h"
#include "utils.h"

static pplx::task<bool> _do_while_iteration(std::function<pplx::task<bool>(void)> func)
{
    pplx::task_completion_event<bool> ev;

    func().then([=](bool task_completed)
    {
        ev.set(task_completed);
    });

    return pplx::create_task(ev);
}

static pplx::task<bool> _do_while_impl(std::function<pplx::task<bool>(void)> func)
{
    return _do_while_iteration(func).then([=](bool continue_next_iteration) -> pplx::task<bool>
    {
        return ((continue_next_iteration == true) ? _do_while_impl(func) : pplx::task_from_result(false));
    });
}


pplx::task<void> async_do_while(std::function<pplx::task<bool>(void)> func)
{
    return _do_while_impl(func).then([](bool)
    {
    });
}


std::string utf16_to_utf8(const std::wstring& w)
{
    return std::wstring_convert<
        std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(w);
}

std::wstring utf8_to_utf16(const std::string& n)
{
    return std::wstring_convert<
        std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(n);
}
