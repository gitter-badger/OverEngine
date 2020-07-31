project "OverEditor"
	if (OverEditorExecHideConsole) then
		kind "WindowedApp"
	else
		kind "ConsoleApp"
	end

	language "C++"
	cppdialect "C++17"

	targetname "OverEditor"
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir("../bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"../OverEngine/src",
		"../OverEditor/res",
		"../OverEngine/vendor",
		"../%{IncludeDir.spdlog}",
		"../%{IncludeDir.glm}",
		"../%{IncludeDir.entt}",
		"../%{IncludeDir.Box2D}"
	}

	links "OverEngine"
	if (DynamicLink) then
		defines "OE_BUILD_SHARED"
	else
		links (LinkLibs)
		defines "OE_BUILD_STATIC"
	end

	filter "system:windows"
		systemversion "latest"
		staticruntime (StaticRuntime)
		files "res/**.rc"


	filter "system:linux"
		pic "on"
		systemversion "latest"
		staticruntime "on"
		links { "dl", "pthread" }
		
	filter "configurations:Debug"
		defines "OE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:DebugOptimized"
		defines "OE_DEBUG"
		runtime "Debug"
		symbols "on"
		optimize "on"

	filter "configurations:Release"
		defines "OE_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "OE_DIST"
		runtime "Release"
		optimize "on"
