# Diligent Engine Vulkan tutorial
This repo contains a very short but complete example about how you can use the 
[Diligent Engine](https://github.com/DiligentGraphics/DiligentEngine) library to blur an image using a headless
`Vulkan` renderer. The project uses [Conan](https://conan.io/) package manager and `CMake` and should be highly 
reproducible. You will need a relatively modern compiler supporting C++20 as I use some newer features
(e.g. [Designated initializers](https://en.cppreference.com/w/cpp/language/aggregate_initialization)) and see no reason 
to lower this requirement.

Motivation
--------------
There are at least a handful of different `Vulkan` tutorials out there, including 
[Vulkan Tutorial](https://vulkan-tutorial.com/), 
[Sascha Willems Vulkan](https://github.com/SaschaWillems/Vulkan),
[LunarG Tuturial](https://vulkan.lunarg.com/doc/view/1.2.154.1/windows/tutorial/html/index.html)
and of course official [documentation and samples](https://www.vulkan.org/learn). These are definitely very good 
resources to learn about `Vulkan` and modern GPUs pipeline in general, but this is a lot to read and digest especially
if you haven't touched any GPU APIs before. Modern GPU APIs like `Vulkan`, `DirectX 12`, `Metal` and `WebGPU` are often 
viewed (not without reason) as **advanced** compared to `OpenGL` for example, and this immediately repels many, making 
them learn `OpenGL` instead. And while `OpenGL` looks much simpler on the surface, if you start asking questions like
**why?** or **how?** this particular `glReadPixels` function works on a deeper level, then you face a much more severe 
problem - opaqueness. And to solve this problem you need to play around with the API, read the docs, stackoverflow, etc. 
regardless of the API you have chosen. 

But! I will not cover such detailed deep dive here in this tutorial because I'm not an expert and this is not my goal.
My goal here is to give a very simple and extensible example of how to implement a headless `Vulkan` renderer without
writing 1000+ lines of code and to leave some room for changes and experimentation if you want to. And here is where a
brilliant graphics abstraction layer `Diligent Engine` comes into play.

## Prerequisites