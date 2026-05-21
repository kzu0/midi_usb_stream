#ifndef MIDI_USB_STREAM_TESTS_H
#define MIDI_USB_STREAM_TESTS_H

#include "midi_usb_stream.h"
#include <random>
#include <cstdio>

static bool midi_usb_encoder_test() {

    uint8_t midi_message[16];
    uint8_t midi_data[16];

    uint8_t data_count;

    midi_usb_packet_t packet;
    uint8_t cable = 5;
    uint8_t ret;

    // Inizializzazione contesto
    midi_usb_stream_ctx_t ctx;
    midi_usb_init_ctx( &ctx );
    midi_usb_set_cable( &ctx, cable );

    /* ------------------------------------------------------------------
     * Note On
     * ---------------------------------------------------------------- */

    printf ("Test encoding / decoding note on message ... \n");

    midi_message[0] = 0x94;
    midi_message[1] = 0x67;
    midi_message[2] = 0x30;
    data_count = 3;

    ret = midi_usb_encode_message( &ctx, midi_message, data_count, &packet );
    printf ("Encoded bytes: %d \n", ret);

    ret = midi_usb_decode_packet( &ctx, packet, midi_data );
    printf ("Decoded bytes: %d \n", ret);

    ret = memcmp((void*)midi_message, (void*)midi_data, data_count);

    if (ret)
    {
        printf ("Test data error: %d \n\n", ret);
        fflush(stdout);
        return false;
    }
    else
    {
        printf ("Test data OK \n\n");
        fflush(stdout);
    }

    /* ------------------------------------------------------------------
     * Program Change
     * ---------------------------------------------------------------- */

    printf ("Test encoding / decoding program change message ... \n");

    midi_message[0] = 0xC3;
    midi_message[1] = 0x15;
    data_count = 2;

    ret = midi_usb_encode_message( &ctx, midi_message, data_count, &packet );
    printf ("Encoded bytes: %d \n", ret);

    ret = midi_usb_decode_packet( &ctx, packet, midi_data );
    printf ("Decoded bytes: %d \n", ret);

    ret = memcmp((void*)midi_message, (void*)midi_data, data_count);

    if (ret)
    {
        printf ("Test data error: %d \n\n", ret);
        fflush(stdout);
        return false;
    }
    else
    {
        printf ("Test data OK \n\n");
        fflush(stdout);
    }

    /* ------------------------------------------------------------------
     * Real time message
     * ---------------------------------------------------------------- */

    printf ("Test encoding / decoding real time message ... \n");

    midi_message[0] = 0xF8;
    data_count = 1;

    ret = midi_usb_encode_message( &ctx, midi_message, data_count, &packet );
    printf ("Encoded bytes: %d \n", ret);

    ret = midi_usb_decode_packet( &ctx, packet, midi_data );
    printf ("Decoded bytes: %d \n", ret);

    ret = memcmp((void*)midi_message, (void*)midi_data, data_count);

    if (ret)
    {
        printf ("Test data error: %d \n\n", ret);
        fflush(stdout);
        return false;
    }
    else
    {
        printf ("Test data OK \n\n");
        fflush(stdout);
    }

    /* ------------------------------------------------------------------
     * Sys Ex
     * ---------------------------------------------------------------- */

    constexpr uint32_t DATA_SIZE = 71;

    printf ("Test encoding / decoding sys ex message ... \n");
    printf ("Sys ex size: %d \n", DATA_SIZE);

    uint8_t raw_data[DATA_SIZE];
    uint8_t encoded_data[DATA_SIZE * 2];
    uint8_t decoded_data[DATA_SIZE];

    for (uint32_t i = 0; i < DATA_SIZE; i++)
    {
        raw_data[i] = rand() & 0x7F; // SOLO data byte validi MIDI
    }

    // Forza start/end corretti
    raw_data[0] = 0xF0;
    raw_data[DATA_SIZE - 1] = 0xF7;

    /* ------------------------------------------------------------------
     * Encode
     * ---------------------------------------------------------------- */

    uint32_t encoded_pos = 0;

    for (uint32_t i = 0; i < DATA_SIZE; i++)
    {
        if ( midi_usb_encode_sysex(&ctx, raw_data[i], &packet))
        {
            memcpy(&encoded_data[encoded_pos], &packet, sizeof(packet));
            encoded_pos += sizeof(packet);
        }
    }

    /* ------------------------------------------------------------------
     * Decode
     * ---------------------------------------------------------------- */

    uint32_t decoded_pos = 0;

    for (uint32_t i = 0; i < encoded_pos; i += 4)
    {
        memcpy(&packet, &encoded_data[i], sizeof(packet));

        data_count = midi_usb_decode_packet( &ctx, packet, midi_data );
        if (data_count)
        {
            memcpy(&decoded_data[decoded_pos], midi_data, data_count);
            decoded_pos += data_count;
        }
    }

    printf ("Encoded bytes: %d \n", encoded_pos);
    printf ("Decoded bytes: %d \n", decoded_pos);

    bool size_ok = (decoded_pos == DATA_SIZE);
    bool data_ok = (memcmp(raw_data, decoded_data, DATA_SIZE) == 0);

    printf ("Size OK: %d \n", size_ok);
    printf ("Data OK: %d \n", data_ok);

    if ((size_ok==false) || (data_ok==false))
    {
        printf ("Test data error \n");
        fflush(stdout);
        return false;
    }
    else
    {
        printf ("Test data OK \n\n");
        fflush(stdout);
    }

    return true;
}


#endif // MIDI_USB_STREAM_TESTS_H
