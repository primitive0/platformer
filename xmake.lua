add_rules("mode.debug", "mode.release")
add_requires("vulkan", "glfw")

target("unnamed-platformer")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("vulkan", "glfw")
    set_languages("c++17")
    if is_mode("release") then
        set_optimize("fastest")
    else
        set_optimize("none")
    end
