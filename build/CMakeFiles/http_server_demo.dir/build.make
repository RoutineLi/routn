# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/rotun-li/routn

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/rotun-li/routn/build

# Include any dependencies generated for this target.
include CMakeFiles/http_server_demo.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/http_server_demo.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/http_server_demo.dir/flags.make

CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.o: CMakeFiles/http_server_demo.dir/flags.make
CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.o: ../examples/http_server_demo.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/rotun-li/routn/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.o"
	/bin/x86_64-linux-gnu-g++-9  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.o -c /home/rotun-li/routn/examples/http_server_demo.cpp

CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.i"
	/bin/x86_64-linux-gnu-g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/rotun-li/routn/examples/http_server_demo.cpp > CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.i

CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.s"
	/bin/x86_64-linux-gnu-g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/rotun-li/routn/examples/http_server_demo.cpp -o CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.s

# Object files for target http_server_demo
http_server_demo_OBJECTS = \
"CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.o"

# External object files for target http_server_demo
http_server_demo_EXTERNAL_OBJECTS =

../bin/http_server_demo: CMakeFiles/http_server_demo.dir/examples/http_server_demo.cpp.o
../bin/http_server_demo: CMakeFiles/http_server_demo.dir/build.make
../bin/http_server_demo: ../lib/libroutn.so
../bin/http_server_demo: CMakeFiles/http_server_demo.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/rotun-li/routn/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/http_server_demo"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/http_server_demo.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/http_server_demo.dir/build: ../bin/http_server_demo

.PHONY : CMakeFiles/http_server_demo.dir/build

CMakeFiles/http_server_demo.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/http_server_demo.dir/cmake_clean.cmake
.PHONY : CMakeFiles/http_server_demo.dir/clean

CMakeFiles/http_server_demo.dir/depend:
	cd /home/rotun-li/routn/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/rotun-li/routn /home/rotun-li/routn /home/rotun-li/routn/build /home/rotun-li/routn/build /home/rotun-li/routn/build/CMakeFiles/http_server_demo.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/http_server_demo.dir/depend

