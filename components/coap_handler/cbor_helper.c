#include "cbor_helper.h"

#include "cbor.h"

size_t cbor_helper_return_uint32(uint8_t *buffer, size_t buffer_size, uint32_t key, uint32_t value)
{
    CborEncoder encoder;
    CborEncoder mapEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);
    // Eine Map mit 1 Eintrag starten
    cbor_encoder_create_map(&encoder, &mapEncoder, 1);
    // Key = key, Value = value
    cbor_encode_uint(&mapEncoder, key);
    cbor_encode_uint(&mapEncoder, value);
    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}