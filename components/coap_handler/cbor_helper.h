#pragma once

#include <stdint.h>
#include <stddef.h>

size_t cbor_helper_return_uint32(uint8_t *buffer, size_t buffer_size, uint32_t key, uint32_t value);