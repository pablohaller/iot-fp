
// Definiciones y estructuras
#define BUFFER_SIZE 20

typedef struct
{
    uint8_t data[BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} circular_buffer_t;

uint8_t buffer_read();
void buffer_print();
void buffer_write(uint8_t value);
void buffer_init();
void init_logger(void);
