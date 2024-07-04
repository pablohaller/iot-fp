# Establecer las fuentes del componente
set(COMPONENT_SRCS "app_main.c" )

# Registrar el componente e incluir los directorios necesarios
idf_component_register(SRCS "app_main.c"
                       INCLUDE_DIRS ".")

# Crear una imagen SPIFFS desde el contenido del directorio 'spiffs'
# que se ajuste a la partición llamada 'storage'. FLASH_IN_PROJECT indica que
# la imagen generada debe ser flasheada cuando se flashea todo el proyecto al
# objetivo con 'idf.py -p PORT flash'.
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)

# Agregar opciones de compilación específicas para este componente
target_compile_options(${COMPONENT_LIB} PRIVATE
                       -Wno-deprecated-declarations)
