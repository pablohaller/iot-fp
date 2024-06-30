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

## API Reference

#### Connect to STA

```http
  GET /sta-connect
```

| Parameter  | Type     | Description                        |
| :--------- | :------- | :--------------------------------- |
| `ssid`     | `string` | SSID of the network to connect     |
| `password` | `string` | Password of the network to connect |

#### Media handler

```http
  POST /media
```

| Parameter | Type                                            | Description                       |
| :-------- | :---------------------------------------------- | :-------------------------------- |
| `action`  | `prev` \| `next` \| `pause` \| `play` \| `stop` | **Required**. Id of item to fetch |

##### Response example

```json
{
  "playing": {
    "id": "",
    "info": ""
  },
  "state": "play"
}
```

#### MQTT handler

```http
  POST /mqtt
```

| Parameter | Type     | Description       |
| :-------- | :------- | :---------------- |
| `broker`  | `string` | Broker URL        |
| `topic`   | `string` | Topic to suscribe |
