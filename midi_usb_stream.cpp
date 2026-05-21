#include "midi_usb_stream.h"

uint8_t midi_usb_decode_packet( midi_usb_stream_ctx_t *ctx, midi_usb_packet_t midi_packet, uint8_t *midi_raw_data )
{
    if ( !ctx || !midi_raw_data )
        return 0;

    // Decodifica USB MIDI -> MIDI
    uint8_t	cn   	= ( midi_packet >> 0x04 ) & 0x0f;	// Cable Number (CN)
    uint8_t	cin  	= ( midi_packet & 0x0f );			// Codex Index Number (CIN)
    uint8_t byte1	= ( midi_packet >> 0x08 ) & 0xff;
    uint8_t	byte2 	= ( midi_packet >> 0x10 ) & 0xff;
    uint8_t	byte3 	= ( midi_packet >> 0x18 ) & 0xff;

    static const uint8_t data_len[ 16 ] = {
        0, 0, 2, 3,   /* 0x0-0x3 */
        3, 1, 2, 3,   /* 0x4-0x7 */
        3, 3, 3, 3,   /* 0x8-0xB */
        2, 2, 3, 1,   /* 0xC-0xF */
    };
    uint8_t len = data_len[ cin ];

    if ( len == 0 )
        return 0;

    // Midi USB
    ctx->cn  = cn;
    ctx->cin = cin;

    // Midi puro
    *( midi_raw_data ) = byte1;
    *( midi_raw_data + 1 ) = ( len >= 2 ) ? byte2 : 0x00;
    *( midi_raw_data + 2 ) = ( len >= 3 ) ? byte3 : 0x00;

    return len;
}

inline midi_usb_packet_t build_packet( uint8_t cn, uint8_t cin, uint8_t byte1, uint8_t byte2, uint8_t byte3 )
{
    return (midi_usb_packet_t)(
        ((cn  & 0x0F) << 4 | (cin & 0x0F))  |
        ((uint32_t)byte1 << 8)              |
        ((uint32_t)byte2 << 16)             |
        ((uint32_t)byte3 << 24));
}

uint8_t midi_usb_encode_message( midi_usb_stream_ctx_t *ctx, uint8_t *midi_raw_data, uint8_t midi_raw_length, midi_usb_packet_t *midi_packet )
{
    if ( !ctx || !midi_raw_data || ( midi_raw_length == 0) || !midi_packet )
        return 0;

    // Ignora messaggi SysEx
    if ( ( *midi_raw_data == 0xF0 ) || ( *midi_raw_data == 0xF7 ) )
        return 0;

    // Codifica MIDI -> USB MIDI
    uint8_t cn      = ctx->cn;
    uint8_t cin     = 0;
    uint8_t byte1   = *( midi_raw_data );
    uint8_t byte2   = ( midi_raw_length >= 2 ) ? *( midi_raw_data + 1 ) : 0x00;
    uint8_t byte3   = ( midi_raw_length >= 3 ) ? *( midi_raw_data + 2 ) : 0x00;
    uint8_t status  = byte1;

    /* ------------------------------------------------------------------
     * System Real Time (0xF8–0xFF)
     * ---------------------------------------------------------------- */
    if ( status >= 0xF8 )
    {
        *midi_packet = build_packet( cn, CIN_SINGLE_BYTE, byte1, 0x00, 0x00 );
        return 4;
    }

    /* ------------------------------------------------------------------
     * System Common (0xF1–0xF6)
     * ---------------------------------------------------------------- */
    if ( status >= 0xF1 && status <= 0xF6 )
    {
        switch ( status )
        {
            // One byte System Common
        case 0xf6:	// Tune Request
            cin = CIN_SYS_COM_1;
            break;

            // Two bytes System Common
        case 0xf1:  // MIDI Time Code Quarter Frame
        case 0xf3:	// Song Select
            cin = CIN_SYS_COM_2;
            break;

            // Three bytes System Common
        case 0xf2:  // Song Position Pointer
            cin = CIN_SYS_COM_3;
            break;

            // Undefined
        case 0xf4:
        case 0xf5:
        default:
            return 0;
        }

        *midi_packet = build_packet( cn, cin, byte1, byte2, byte3 );
        return 4;
    }

    /* ------------------------------------------------------------------
     * Channel message (0x80–0xEF)
     * ---------------------------------------------------------------- */
    if ( status >= 0x80 && status <= 0xEF )
    {
        *midi_packet = build_packet( cn, ( ( status & 0xf0 ) >> 4 ), byte1, byte2, byte3 );
        return 4;
    }

    return 0;
}

uint8_t midi_usb_encode_sysex( midi_usb_stream_ctx_t *ctx, uint8_t byte, midi_usb_packet_t *midi_packet )
{
    if ( !ctx || !midi_packet || ctx->cn >= USB_MIDI_MAX_CABLES )
        return 0;

    // Ignora messaggi non SysEx
    if ( ( byte >= 0xF1 && byte <= 0xF6 ) || ( byte >= 0xF8 ) )
        return 0;

    uint8_t cn = ctx->cn;

    midi_usb_cable_data_t *cable = &ctx->cable[ cn ];

    /* ------------------------------------------------------------------
     * SysEx Start (0xF0)
     * Resetta buffer
     * ---------------------------------------------------------------- */
    if ( byte == 0xF0 )
    {
        // Resetto la posizione sul buffer e setto lo stato sysex
        cable->sysex_data[ 0 ] = byte;
        cable->sysex_count = 1;
        cable->sysex = true;
        return 0;
    }

    /* ------------------------------------------------------------------
     * SysEx End (0xF7)
    * Il CIN dipende da quanti byte sono nel buffer al momento di 0xF7.
    * Dopo l'emissione di un pacchetto CIN 0x04, il buffer è vuoto
    * (sysex_count == 0), quindi tutti e tre i casi sono raggiungibili:
    *
    *   sysex_count == 1  →  solo 0xF7                  → CIN 0x05
    *   sysex_count == 2  →  1 byte  + 0xF7             → CIN 0x06
    *   sysex_count == 3  →  2 byte  + 0xF7             → CIN 0x07
    *
    * Esempio CIN 0x05: F0 xx xx F7
    *   [F0 xx xx] → CIN 0x04 emesso, sysex_count = 0
    *   [F7]       → sysex_count = 1 → CIN 0x05
     * ------------------------------------------------------------------ */
    else if ( byte == 0xF7 && cable->sysex )
    {
        cable->sysex_data[ cable->sysex_count++ ] = byte;

        static const uint8_t cin_end[ 4 ] = {
            0x00, CIN_SYSEX_END_1, CIN_SYSEX_END_2, CIN_SYSEX_END_3
        };

        uint8_t cin = cin_end[ cable->sysex_count ];
        uint8_t len = cable->sysex_count;

        // Chiudo il pacchetto
        *midi_packet = build_packet( cn, cin,
                                    cable->sysex_data[ 0 ],
                                    len >= 2 ? cable->sysex_data[ 1 ] : 0x00,
                                    len >= 3 ? cable->sysex_data[ 2 ] : 0x00 );

        // Resetto la posizione sul buffer e resetto lo stato sysex
        cable->sysex_count = 0;
        cable->sysex = false;
        return 4;
    }

    /* ------------------------------------------------------------------
     * SysEx Data
     * Accumula; emette un pacchetto CIN 0x04 ogni 3 byte.
     * ---------------------------------------------------------------- */
    else if ( cable->sysex )
    {
        cable->sysex_data[ cable->sysex_count++ ] = byte;

        if ( cable->sysex_count >= 3 )
        {
            *midi_packet = build_packet( cn, CIN_SYSEX_START,
                                        cable->sysex_data[ 0 ],
                                        cable->sysex_data[ 1 ],
                                        cable->sysex_data[ 2 ] );

            // Resetto solo la posizione sul buffer
            cable->sysex_count = 0;
            return 4;
        }
    }

    return 0;
}

void midi_usb_init_ctx( midi_usb_stream_ctx_t *ctx )
{
    if ( !ctx )
        return;

    for ( uint8_t i = 0; i < USB_MIDI_MAX_CABLES; i ++ )
    {
        ctx->cable[i].sysex = false;
        ctx->cable[i].sysex_count = 0;
        ctx->cable[i].sysex_data[ 0 ] = 0;
        ctx->cable[i].sysex_data[ 1 ] = 0;
        ctx->cable[i].sysex_data[ 2 ] = 0;
    }
}

