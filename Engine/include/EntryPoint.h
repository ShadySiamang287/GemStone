#pragma once

#include "Core/Application.h"

#include <cstdio>
#include <iostream>
#include <memory>
void CreateDebugConsole()
{
#ifdef _DEBUG
    if (AllocConsole())
    {
        FILE* stream = nullptr;

        freopen_s(
            &stream,
            "CONOUT$",
            "w",
            stdout);

        freopen_s(
            &stream,
            "CONOUT$",
            "w",
            stderr);

        freopen_s(
            &stream,
            "CONIN$",
            "r",
            stdin);

        std::cout.clear();
        std::cerr.clear();
        std::cin.clear();
    }
#endif
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    CreateDebugConsole();
	std::unique_ptr<GEM::Application> app = std::unique_ptr<GEM::Application>(GEM::CreateApplication(instance));
	app->Run();
}