cmake_minimum_required(VERSION 3.20)

project(Vectorial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)

add_executable(vectorial
    resources.qrc
    src/main.cpp
    src/Vector3.cpp
    src/VectorExpression.cpp
    src/AppModel.cpp
    src/Theme.cpp
    src/VectorTextEdit.cpp
    src/VectorCard.cpp
    src/OperationCard.cpp
    src/Graph2DWidget.cpp
    src/GLWidget.cpp
    src/MainWindow.cpp
    src/Vector3.h
    src/VectorExpression.h
    src/GraphTypes.h
    src/VectorEntry.h
    src/AppModel.h
    src/Theme.h
    src/VectorTextEdit.h
    src/VectorCard.h
    src/OperationCard.h
    src/Graph2DWidget.h
    src/GLWidget.h
    src/MainWindow.h
)
target_include_directories(vectorial PRIVATE src)
target_link_libraries(vectorial PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
target_compile_options(vectorial PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()

add_executable(test_vector3
    tests/test_vector3.cpp
    src/Vector3.cpp
    src/Vector3.h
)
target_include_directories(test_vector3 PRIVATE src)
target_compile_options(test_vector3 PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME Vector3Tests COMMAND test_vector3)

add_executable(test_expressions
    tests/test_expressions.cpp
    src/VectorExpression.cpp
    src/VectorExpression.h
    src/Vector3.cpp
    src/Vector3.h
)
target_include_directories(test_expressions PRIVATE src)
target_link_libraries(test_expressions PRIVATE Qt6::Core)
target_compile_options(test_expressions PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME ExpressionTests COMMAND test_expressions)
