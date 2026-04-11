target("vfilesystem")
	set_kind("static")
	if is_plat("android") then
		add_cflags("-fPIC")
		add_cxflags("-fPIC")
		add_syslinks("android")
	end

	add_headerfiles("include/(vfilesystem/**.hpp)")
	add_includedirs("include", {public = true})

	add_files("src/**.cpp")

	add_deps("vbase", {public = true})

	-- set target directory
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/vfilesystem")
