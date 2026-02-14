target("vfilesystem")
	set_kind("static")

	add_headerfiles("include/(vfilesystem/**.hpp)")
	add_includedirs("include", {public = true})

	add_files("src/**.cpp")

	add_deps("vbase", {public = true})

	-- set target directory
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/vfilesystem")