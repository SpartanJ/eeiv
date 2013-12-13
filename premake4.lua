function newplatform(plf)
	local name = plf.name
	local description = plf.description

	-- Register new platform
	premake.platforms[name] = {
		cfgsuffix = "_"..name,
		iscrosscompiler = true
	}

	-- Allow use of new platform in --platfroms
	table.insert(premake.option.list["platform"].allowed, { name, description })
	table.insert(premake.fields.platforms.allowed, name)

	-- Add compiler support
	premake.gcc.platforms[name] = plf.gcc
end

function newgcctoolchain(toolchain)
	newplatform {
		name = toolchain.name,
		description = toolchain.description,
		gcc = {
			cc = toolchain.prefix .. "gcc",
			cxx = toolchain.prefix .. "g++",
			ar = toolchain.prefix .. "ar",
			cppflags = "-MMD " .. toolchain.cppflags
		}
	}
end

newplatform {
	name = "clang",
	description = "Clang",
	gcc = {
		cc = "clang",
		cxx = "clang++",
		ar = "ar",
		cppflags = "-MMD "
	}
}

newgcctoolchain {
	name = "mingw32",
	description = "Mingw32 to cross-compile windows binaries from *nix",
	prefix = "i686-w64-mingw32-",
	cppflags = ""
}

newgcctoolchain {
	name ="android-arm7",
	description = "Android ARMv7 (not implemented)",
	prefix = iif( os.getenv("ANDROID_NDK"), os.getenv("ANDROID_NDK"), "" ) .. "arm-linux-androideabi-",
	cppflags = "-MMD -arch=armv7 -march=armv7 -marm -mcpu=cortex-a8"
}

toolchain_path = os.getenv("TOOLCHAINPATH")

if not toolchain_path then
	toolchain_path = ""
end

if _OPTIONS.platform then
	-- overwrite the native platform with the options::platform
	premake.gcc.platforms['Native'] = premake.gcc.platforms[_OPTIONS.platform]
end

function os.get_real()
	if 	_OPTIONS.platform == "ios-arm7" or 
		_OPTIONS.platform == "ios-x86" or
		_OPTIONS.platform == "ios-cross-arm7" then
		return "ios"
	end
	
	if _OPTIONS.platform == "android-arm7" then
		return "android"
	end
	
	if 	_OPTIONS.platform == "mingw32" then
		return _OPTIONS.platform
	end

	return os.get()
end

function os.is_real( os_name )
	return os.get_real() == os_name
end

function print_table( table_ref )
	for _, value in pairs( table_ref ) do
		print(value)
	end
end

function args_contains( element )
	return table.contains( _ARGS, element )
end

function multiple_insert( parent_table, insert_table )
	for _, value in pairs( insert_table ) do
		table.insert( parent_table, value )
	end
end

function os_findlib( name )
	if os.is("macosx") then
		local path = os.findlib( name .. ".framework" )
		
		if path then
			return path
		end
	end
	
	return os.findlib( name )
end

function get_backend_link_name( name )
	if os.is("macosx") then
		local fname = name .. ".framework"
		
		if os.findlib( fname ) then -- Search for the framework
			return fname
		end
	end
	
	return name
end

function string.starts(String,Start)
	if ( _ACTION ) then
		return string.sub(String,1,string.len(Start))==Start
	end
	
	return false
end

function is_vs()
	return ( string.starts(_ACTION,"vs") )
end

function conf_warnings()
	if not is_vs() then
		buildoptions{ "-Wall -Wno-long-long" }
	else
		defines { "_SCL_SECURE_NO_WARNINGS" }
	end
end

function add_cross_config_links()
	if not is_vs() then
		if os.is_real("mingw32") or os.is_real("ios") then -- if is crosscompiling from *nix
			linkoptions { "-static-libgcc", "-static-libstdc++" }
		end
	end
end

solution "eeiv"
	location("./make/" .. os.get() .. "/")
	targetdir("./")
	objdir("obj/" .. os.get() .. "/")
	configurations { "debug", "release" }

	project "eeiv"
		kind "WindowedApp"
		language "C++"
		
		files { "src/**.cpp" }
		includedirs { "include", "src" }
		
		add_cross_config_links()

		configuration "debug"
			defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
			flags { "Symbols" }
			targetname "eeiv-debug"
			links{ "eepp-debug" }
			conf_warnings()

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "eeiv"
			links{ "eepp" }
			conf_warnings()
