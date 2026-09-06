#pragma once

#include <LoopbackStream.h>
#include <freertos/FreeRTOS.h>

/**
 * Same as LoopbackStream, but it includes extra methods to handle what we need from it
 *  basically we're just completing the existing usages of Serial
 *
 * Also thread-safe, which the base class is not: when used as the WiFi
 * bridge's rx/tx buffer, AsyncTCP writes from its own FreeRTOS task
 * (see AsyncTCP.cpp's _async_service_task, a separate core from Arduino's
 * loop()) while the ARQ protocol reads from the loop() task. Two unlocked
 * threads mutating LoopbackStream's shared pos/size counters concurrently
 * corrupts them — this was the real cause of the high corrupted/retransmit
 * counts SimHub reported over WiFi, independent of buffer size. A short
 * critical section (portMUX) around each buffer access is cheap enough to
 * not matter and fixes it completely.
 */
class FullLoopbackStream : public LoopbackStream
{
public:
    FullLoopbackStream(uint16_t buffer_size = LoopbackStream::DEFAULT_SIZE);

    size_t write(const uint8_t *buffer, size_t size);
    size_t write(const char *buffer, size_t size);
    size_t write(const char *str);
    using LoopbackStream::write;

    size_t write(uint8_t) override;
    int availableForWrite() override;
    int available() override;
    bool contains(char) override;
    int read() override;
    int peek() override;

private:
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
};
