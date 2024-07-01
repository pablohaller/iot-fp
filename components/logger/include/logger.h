#define LOGGER_H

#include <stdint.h>

#define BUFFER_SIZE 20

typedef enum
{
    PLAY = 0,
    PAUSE = 1,
    NEXT = 2,
    PREVIOUS = 3,
    STOP = 4,
} EventType;

typedef struct
{
    EventType event;
    uint8_t song_id;
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
