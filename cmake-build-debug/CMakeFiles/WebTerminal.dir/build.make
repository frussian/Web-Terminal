# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


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
CMAKE_COMMAND = /home/frussian/bin/cmake

# The command to remove a file.
RM = /home/frussian/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug"

# Include any dependencies generated for this target.
include CMakeFiles/WebTerminal.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/WebTerminal.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/WebTerminal.dir/flags.make

CMakeFiles/WebTerminal.dir/main.c.o: CMakeFiles/WebTerminal.dir/flags.make
CMakeFiles/WebTerminal.dir/main.c.o: ../main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/WebTerminal.dir/main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/WebTerminal.dir/main.c.o   -c "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/main.c"

CMakeFiles/WebTerminal.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/WebTerminal.dir/main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/main.c" > CMakeFiles/WebTerminal.dir/main.c.i

CMakeFiles/WebTerminal.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/WebTerminal.dir/main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/main.c" -o CMakeFiles/WebTerminal.dir/main.c.s

CMakeFiles/WebTerminal.dir/server/httpd.c.o: CMakeFiles/WebTerminal.dir/flags.make
CMakeFiles/WebTerminal.dir/server/httpd.c.o: ../server/httpd.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/WebTerminal.dir/server/httpd.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/WebTerminal.dir/server/httpd.c.o   -c "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/server/httpd.c"

CMakeFiles/WebTerminal.dir/server/httpd.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/WebTerminal.dir/server/httpd.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/server/httpd.c" > CMakeFiles/WebTerminal.dir/server/httpd.c.i

CMakeFiles/WebTerminal.dir/server/httpd.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/WebTerminal.dir/server/httpd.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/server/httpd.c" -o CMakeFiles/WebTerminal.dir/server/httpd.c.s

CMakeFiles/WebTerminal.dir/tty/tty.c.o: CMakeFiles/WebTerminal.dir/flags.make
CMakeFiles/WebTerminal.dir/tty/tty.c.o: ../tty/tty.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/WebTerminal.dir/tty/tty.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/WebTerminal.dir/tty/tty.c.o   -c "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/tty/tty.c"

CMakeFiles/WebTerminal.dir/tty/tty.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/WebTerminal.dir/tty/tty.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/tty/tty.c" > CMakeFiles/WebTerminal.dir/tty/tty.c.i

CMakeFiles/WebTerminal.dir/tty/tty.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/WebTerminal.dir/tty/tty.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/tty/tty.c" -o CMakeFiles/WebTerminal.dir/tty/tty.c.s

CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.o: CMakeFiles/WebTerminal.dir/flags.make
CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.o: ../esc_parser/esc_parser.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.o   -c "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/esc_parser/esc_parser.c"

CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/esc_parser/esc_parser.c" > CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.i

CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/esc_parser/esc_parser.c" -o CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.s

# Object files for target WebTerminal
WebTerminal_OBJECTS = \
"CMakeFiles/WebTerminal.dir/main.c.o" \
"CMakeFiles/WebTerminal.dir/server/httpd.c.o" \
"CMakeFiles/WebTerminal.dir/tty/tty.c.o" \
"CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.o"

# External object files for target WebTerminal
WebTerminal_EXTERNAL_OBJECTS =

WebTerminal: CMakeFiles/WebTerminal.dir/main.c.o
WebTerminal: CMakeFiles/WebTerminal.dir/server/httpd.c.o
WebTerminal: CMakeFiles/WebTerminal.dir/tty/tty.c.o
WebTerminal: CMakeFiles/WebTerminal.dir/esc_parser/esc_parser.c.o
WebTerminal: CMakeFiles/WebTerminal.dir/build.make
WebTerminal: CMakeFiles/WebTerminal.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_5) "Linking C executable WebTerminal"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/WebTerminal.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/WebTerminal.dir/build: WebTerminal

.PHONY : CMakeFiles/WebTerminal.dir/build

CMakeFiles/WebTerminal.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/WebTerminal.dir/cmake_clean.cmake
.PHONY : CMakeFiles/WebTerminal.dir/clean

CMakeFiles/WebTerminal.dir/depend:
	cd "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal" "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal" "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug" "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug" "/mnt/c/Users/Anton/Desktop/BMSTU/Project/Practice 2021/Web Terminal/cmake-build-debug/CMakeFiles/WebTerminal.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/WebTerminal.dir/depend

