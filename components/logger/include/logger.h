#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

#define BUFFER_SIZE 20

typedef enum
{
    PLAY_PAUSE = 0,
    NEXT = 1,
    PREVIOUS = 2,
    STOP = 3,
    VOLUME_UP= 4,
    VOLUME_DOWN= 5
} EventType;


typedef struct
{
    EventType event;
    uint8_t song_id;
    int64_t timestamp;
} buffer_entry_t;

typedef struct
{
    buffer_entry_t data[BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} circular_buffer_t;

char *buffer_to_json(void);
const char *getEventName(EventType event);
uint8_t buffer_read(buffer_entry_t *entry);
void buffer_print();
void buffer_write(EventType event, uint8_t song_id);
void buffer_init(void);
void init_logger(void);
circular_buffer_t get_buffer_from_nvs(void);
void ntp_sync_time(void);
uint8_t get_last_entry(buffer_entry_t *entry);

#endif // LOGGER_H
