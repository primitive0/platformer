function vulkan_sdk_name()
    if is_os("windows") then
        return "vulkansdk"
    else
        return "vulkan"
    end
end

add_rules("mode.debug", "mode.release")
add_requires("glfw", vulkan_sdk_name())


target("unnamed-platformer")
    set_kind("binary")
    add_files("src/**.cpp")
--     add_sysincludedirs("include")
    add_packages("glfw", vulkan_sdk_name())
    set_languages("c++17")
    if is_mode("release") then
        set_optimize("fastest")
    else
        set_optimize("none")
    end
