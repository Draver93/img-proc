/*
 * Standard Precompiled Header
 * ===========================
 * 
 * Contains common includes and external library headers used throughout
 * the image deinterlace tool.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef STD_AFX_H
#define STD_AFX_H


#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
}

#define GLAD_GLX 0
extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>


#endif //!STD_AFX_H