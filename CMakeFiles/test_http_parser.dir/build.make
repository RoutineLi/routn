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
CMAKE_BINARY_DIR = /home/rotun-li/routn

# Include any dependencies generated for this target.
include CMakeFiles/test_http_parser.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test_http_parser.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_http_parser.dir/flags.make

CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.o: CMakeFiles/test_http_parser.dir/flags.make
CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.o: test/test_http_parser.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/rotun-li/routn/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.o -c /home/rotun-li/routn/test/test_http_parser.cpp

CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/rotun-li/routn/test/test_http_parser.cpp > CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.i

CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/rotun-li/routn/test/test_http_parser.cpp -o CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.s

# Object files for target test_http_parser
test_http_parser_OBJECTS = \
"CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.o"

# External object files for target test_http_parser
test_http_parser_EXTERNAL_OBJECTS =

bin/test_http_parser: CMakeFiles/test_http_parser.dir/test/test_http_parser.cpp.o
bin/test_http_parser: CMakeFiles/test_http_parser.dir/build.make
bin/test_http_parser: lib/libroutn.so
bin/test_http_parser: CMakeFiles/test_http_parser.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/rotun-li/routn/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/test_http_parser"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_http_parser.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_http_parser.dir/build: bin/test_http_parser

.PHONY : CMakeFiles/test_http_parser.dir/build

CMakeFiles/test_http_parser.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_http_parser.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_http_parser.dir/clean

CMakeFiles/test_http_parser.dir/depend:
	cd /home/rotun-li/routn && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/rotun-li/routn /home/rotun-li/routn /home/rotun-li/routn /home/rotun-li/routn /home/rotun-li/routn/CMakeFiles/test_http_parser.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_http_parser.dir/depend

