# Diligent Engine Vulkan Tutorial
This repo contains a very short but complete example about how you can use the 
[Diligent Engine](https://github.com/DiligentGraphics/DiligentEngine) library to blur an image using a headless
`Vulkan` renderer. The project uses [Conan](https://conan.io/) package manager and `CMake` and should be highly 
reproducible. You will need a relatively modern compiler supporting C++20 as I use some newer features
(e.g. [Designated initializers](https://en.cppreference.com/w/cpp/language/aggregate_initialization)) and see no reason 
to lower this requirement.

Motivation
---
There are at least a handful of different `Vulkan` tutorials out there, including 
[Vulkan Tutorial](https://vulkan-tutorial.com/), 
[Sascha Willems Vulkan](https://github.com/SaschaWillems/Vulkan),
[LunarG Tutorial](https://vulkan.lunarg.com/doc/view/1.2.154.1/windows/tutorial/html/index.html)
and of course official [documentation and samples](https://www.vulkan.org/learn). These are definitely very good 
resources to learn about Vulkan and modern GPUs pipeline in general, but this is a lot to read and digest especially
if you haven't touched any GPU APIs before. Modern GPU APIs like `Vulkan`, `DirectX 12`, `Metal` and `WebGPU` are often 
viewed (not without reason) as **advanced** compared to OpenGL for example, and this immediately repels many, making 
them learn `OpenGL` instead. And while OpenGL looks much simpler on the surface, if you start asking questions like
**why?** or **how?** this particular `glReadPixels` function works on a deeper level, then you face a much more severe 
problem - opaqueness. And to solve this problem you need to play around with the API, read the docs, stackoverflow, etc. 
regardless of the API you have chosen.

But! I will not cover such detailed deep dive here in this tutorial because I'm not an expert and this is not my goal.
My goal here is to give a very simple and extensible example of how to implement a headless Vulkan renderer without
writing 1000+ lines of code and to leave some room for changes and experimentation if you want to. And here is where a
brilliant graphics abstraction layer `Diligent Engine` comes into play.

Building
---
To build this tutorial you will need:
* Windows, macOS or Linux
* C++20 compiler
* `Python` >= 3.5
* `CMake` >= 3.15
* `Conan` >= 1.33
* Build system like `Visual Studio`, `Make` or `Ninja`

If you don't have Python you can install it from the [official website](https://www.python.org/downloads/) or any other 
way you may prefer.  
The same goes for CMake - [official website](https://cmake.org/download/).

To install Conan with Python >= 3.5 you can simply get it via the `pip` by running `pip install conan` or instead
following the [official documentation](https://docs.conan.io/en/latest/installation.html) to use some other method.  
**You can also specify the location where the `.conan` caches directory will be created by setting environment variable
`CONAN_USER_HOME` see [reference](https://docs.conan.io/en/latest/reference/env_vars.html#conan-user-home). 
This may be useful if you have another `.conan` cache and don't want to interfere with it.**

Now you can download this repo either by using git
```
git clone https://github.com/MekaGem/diligent-engine-tutorial.git
```
or simply by downloading the [repo archive](https://github.com/MekaGem/diligent-engine-tutorial/archive/refs/heads/main.zip).

To build the tutorial itself you need to run the following right inside the repo directory:
* On Windows with `Visual Studio` CMake generator (usually the default option)
```
conan install . --build=missing -if build 
cmake -S . -B build
cmake --build build --config Release
```
* On macOS and Linux using `make` or `ninja`
```
conan install . --build=missing -if build 
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

And finally, you can now run the application at `build\bin\blur.exe` (Windows) or `build/bin/blur` (macOS/Linux) and 
check that it works!