cmake_minimum_required(VERSION 3.16)
project(ProcLaunch LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Создаем статическую библиотеку из файла src/proclaunch.c
add_library(proclaunch STATIC src/proclaunch.c)

# Указываем, что заголовочные файлы лежат в папке include
target_include_directories(proclaunch PUBLIC include)

# Создаем исполняемый файл для тестов
add_executable(proctest test/proctest.c)

# Линкуем тестовую утилиту с нашей библиотекой
target_link_libraries(proctest PRIVATE proclaunch)

# Включаем предупреждения компилятора для GCC/Clang (Linux)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(proclaunch PRIVATE -Wall -Wextra)
  target_compile_options(proctest PRIVATE -Wall -Wextra)
endif()
