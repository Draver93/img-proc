workspace "img-deinterlace"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    startproject "imgs-deinterlace"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "img-deinterlace"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    staticruntime "On"
    dependson { "ffmpeg" }

    -- Global defines for the entire project
    defines { "NOMINMAX" }

    files { "src/**.h", "src/**.cpp" }
    includedirs {
        "external/ffmpeg",
        "external/ffmpeg/include"
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
        links {
            "avutil",
            "avcodec",
            "avformat",
            "swscale",
            "swresample",
            "avfilter"
        }

    filter { "system:linux" }
        defines { "LINUX" }
        links {
            "avutil",
            "avcodec",
            "avformat",
            "swscale",
            "swresample",
            "avfilter"
        }