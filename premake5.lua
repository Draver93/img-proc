workspace "img_deinterlace"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    startproject "img_deinterlace"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

dofile("external/glfw-premake5.lua")

project "img_deinterlace"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    staticruntime "On"
    dependson { "ffmpeg" }
    dependson { "glfw" }

    -- Global defines for the entire project
    defines { "NOMINMAX" }

    files { "src/**.h", "src/**.cpp", "external/glad/src/glad.c" }
    includedirs {
        "src",
        "external/ffmpeg/build/include",
        "external/glfw/include",
        "external/glad/include"
    }
    libdirs { 
        "external/ffmpeg/build/lib" 
    }

    filter { "configurations:Debug" }
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"

    filter { "configurations:Debug", "system:windows" }
        buildoptions { "/MTd" }
    filter { "configurations:Release", "system:windows" }
        buildoptions { "/MT" }

    filter { "configurations:Debug", "system:linux"}
        buildoptions { "-static-libgcc", "-static-libstdc++", "-g" }
    filter { "configurations:Release", "system:linux"}
        buildoptions { "-static-libgcc", "-static-libstdc++" }


    filter { "system:windows" }
        defines { "WINDOWS" }
        files { "external/glad/src/glad_wgl.c" }
        links {
            "avutil",
            "avcodec",
            "avdevice",
            "avformat",
            "swscale",
            "swresample",
            "avfilter",
            "glfw",
            "opengl32"
        }

    filter { "system:linux" }
        defines { "LINUX" }
        files { "external/glad/src/glad_glx.c" }
        links {
            "avfilter",
            "avformat",
            "avcodec",
            "swresample",
            "swscale",
            "avutil",
            "m",
            "z",
            "pthread",
            "dl",
            "GL",
            "tbb",
            "X11",
            "Xrandr",
            "Xi",
            "Xxf86vm",
            "Xcursor",
            "glfw"
        }