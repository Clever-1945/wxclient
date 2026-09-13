cmake_minimum_required(VERSION 3.20)
project(wxclient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Кодировка UTF-8
add_compile_options("$<$<CXX_COMPILER_ID:GNU>:-finput-charset=UTF-8;-fexec-charset=UTF-8>")

# --- НАСТРОЙКА ПУТЕЙ И ПОДКЛЮЧЕНИЕ ЗАГОЛОВОЧНЫХ ФАЙЛОВ
include_directories(
        "D:/projects/cpp/src/wxWidgets_3_3_3/include"
        "D:/projects/cpp/src/wxWidgets_3_3_3/lib/gcc_lib/mswu"
        "C:/msys64/ucrt64/include/quickjs"
)

# --- НАСТРОЙКА ПУТЕЙ К БИБЛИОТЕКАМ
link_directories(
        "D:/projects/cpp/src/wxWidgets_3_3_3/lib/gcc_lib"
        "C:/msys64/ucrt64/lib/quickjs"
)

# Статическая линковка рантайма
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

# Флаг для скрытия консоли Windows (-mwindows)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")

# --- КОМПИЛЯЦИЯ РЕСУРСА
set(RESOURCE_FILES "resources/resource.rc")

# --- ОБЪЯВЛЕНИЕ ИСПОЛНЯЕМОГО ФАЙЛА ---
add_executable(wxclient main.cpp ${RESOURCE_FILES})

target_include_directories(wxclient PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        )

# --- ПОДКЛЮЧЕНИЕ БИБЛИОТЕК
target_link_libraries(wxclient
        # wxWidgets
        wxmsw33u_core
        wxbase33u
        wxtiff
        wxjpeg
        wxpng
        wxzlib

        # QuickJS
        quickjs

        # Системные библиотеки Windows
        kernel32
        user32
        gdi32
        comdlg32
        winspool
        winmm
        shell32
        comctl32
        ole32
        oleaut32
        uuid
        rpcrt4
        advapi32
        wsock32
        shlwapi
        uxtheme
        version
        gdiplus
        oleacc
        msimg32
        )
