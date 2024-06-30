# IOT-Embeb Final Project

## cJSON

### Adding cJSON

```
git submodule add https://github.com/DaveGamble/cJSON.git components/cJSON

git submodule update --init --recursive
```

### Updating cJSON

Change the contents from `components/cJSON/CMakeLists.txt` to:

```
idf_component_register(SRCS "cJSON.c"
                    INCLUDE_DIRS ".")
```
