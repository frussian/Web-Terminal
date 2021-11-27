cmake_minimum_required(VERSION 3.15)
project(WebTerminal)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99") #TODO: c compiler
set(SOURCE_FILES main.c server/httpd.c server/httpd.h tty/tty.c tty/tty.h esc_parser/esc_parser.c esc_parser/esc_parser.h)
include_directories(WebTerminal
        PUBLIC
        tty
        esc_parser
        server
)
add_executable(WebTerminal ${SOURCE_FILES})