cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(proyecto-final)

# Lista de directorios de componentes adicionales
list(APPEND EXTRA_COMPONENT_DIRS components/cJSON)
