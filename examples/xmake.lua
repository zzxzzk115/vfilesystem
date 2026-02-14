local function add_example(name, src)
    target(name)
        set_kind("binary")
        add_files(src)
        add_includedirs("..", {public=false})
        add_deps("vfilesystem")
        -- set target directory
        set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/" .. name)
        set_rundir(get_config("vfs_project_dir")) -- set runtime directory to project root for easier access files
end

add_example("vfilesystem_example_01_physical_read", "01_physical_read/main.cpp")
add_example("vfilesystem_example_02_vfs_mount", "02_vfs_mount/main.cpp")
add_example("vfilesystem_example_03_memory_fs", "03_memory_fs/main.cpp")
