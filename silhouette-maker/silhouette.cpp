#include <assimp/DefaultLogger.hpp>

#include "window.h"
#include "d3d11/directx_wrapper.h"
#include <cstdlib>
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR pCmdLine, int nCmdShow) {
    std::srand(std::time(nullptr));
	
    Assimp::DefaultLogger::create("log.txt", Assimp::Logger::VERBOSE);

    window current_window = window(hInstance, nCmdShow);
    directx_wrapper d3d11_wrapper = directx_wrapper(current_window.getHWND());

    current_window.run(
            std::bind(&directx_wrapper::render_frame, &d3d11_wrapper),
            std::bind(&directx_wrapper::clean, &d3d11_wrapper)
            );

    Assimp::DefaultLogger::kill();

    return 0;
}
