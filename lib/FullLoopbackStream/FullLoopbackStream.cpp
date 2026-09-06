#include <FullLoopbackStream.h>

FullLoopbackStream::FullLoopbackStream(uint16_t buffer_size) : LoopbackStream(buffer_size){};

size_t FullLoopbackStream::write(const char *str)
{
    if (str == NULL)
        return 0;
    return write((const uint8_t *)str, strlen(str));
}

size_t FullLoopbackStream::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;
    while (size--)
    {
        if (write(*buffer++))
            n++;
        else
            break;
    }
    return n;
}

// Thread-safe overrides — see the class comment in FullLoopbackStream.h for
// why this locking exists. Each critical section is a handful of pointer/
// integer ops, so contention cost is negligible even under bursty traffic.
size_t FullLoopbackStream::write(uint8_t v)
{
    portENTER_CRITICAL(&mux);
    size_t r = LoopbackStream::write(v);
    portEXIT_CRITICAL(&mux);
    return r;
}

int FullLoopbackStream::availableForWrite()
{
    portENTER_CRITICAL(&mux);
    int r = LoopbackStream::availableForWrite();
    portEXIT_CRITICAL(&mux);
    return r;
}

int FullLoopbackStream::available()
{
    portENTER_CRITICAL(&mux);
    int r = LoopbackStream::available();
    portEXIT_CRITICAL(&mux);
    return r;
}

bool FullLoopbackStream::contains(char ch)
{
    portENTER_CRITICAL(&mux);
    bool r = LoopbackStream::contains(ch);
    portEXIT_CRITICAL(&mux);
    return r;
}

int FullLoopbackStream::read()
{
    portENTER_CRITICAL(&mux);
    int r = LoopbackStream::read();
    portEXIT_CRITICAL(&mux);
    return r;
}

int FullLoopbackStream::peek()
{
    portENTER_CRITICAL(&mux);
    int r = LoopbackStream::peek();
    portEXIT_CRITICAL(&mux);
    return r;
}