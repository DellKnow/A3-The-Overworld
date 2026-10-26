/*
 *  CSCI 441, Computer Graphics, Fall 2026
 *
 *  Project: A3
 *  File: main.cpp
 *
 *  Description:
 *      This file contains the basic setup to draw character in a 3D world.
 *
 *  Author: Dr. Paone, Colorado School of Mines
 *
 */

#define CSCI441_OPENGL_ENGINE_IMPLEMENTATION
#include "A3Engine.h"

#include <CSCI441/LogUtils.hpp>

///*****************************************************************************
//
// Our main function
int main() {
    CSCI441::LogUtils::setLogFilename("A3.log");
    CSCI441::LogUtils::resetLogFile();

    const auto a3Engine = new A3Engine();
    a3Engine->initialize();
    if (a3Engine->getError() == CSCI441::OpenGLEngine::OPENGL_ENGINE_ERROR_NO_ERROR) {
        a3Engine->run();
    }
    a3Engine->shutdown();
    delete a3Engine;

    return EXIT_SUCCESS;
}
