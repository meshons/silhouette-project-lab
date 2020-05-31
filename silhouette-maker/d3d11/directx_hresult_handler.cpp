#include "directx_hresult_handler.h"

#include <iostream>
#include <windows.h>
#include <boost/stacktrace.hpp>

directx_hresult_handler & operator<<(directx_hresult_handler & handler, const HRESULT & result) {
    if (result != S_OK) {
        std::string message = std::system_category().message(result);

        std::cout << boost::stacktrace::stacktrace();

        throw std::runtime_error("Error during directx command: " + message);
    }

    return handler;
}