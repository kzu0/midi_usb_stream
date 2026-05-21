#ifndef MIDI_USB_STREAM_H
#define MIDI_USB_STREAM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_MIDI_MAX_CABLES 16

typedef enum
{
    CIN_MISCELLANEOUS       = 0x00,  /* Riservato                        */
    CIN_CABLE_EVENT         = 0x01,  /* Riservato                        */
    CIN_SYS_COM_2           = 0x02,  /* System Common 2 byte             */
    CIN_SYS_COM_3           = 0x03,  /* System Common 3 byte             */
    CIN_SYSEX_START         = 0x04,  /* SysEx inizia o continua          */
    CIN_SYSEX_END_1         = 0x05,  /* SysEx termina con 1 byte         */
    CIN_SYSEX_END_2         = 0x06,  /* SysEx termina con 2 byte         */
    CIN_SYSEX_END_3         = 0x07,  /* SysEx termina con 3 byte         */
    CIN_NOTE_OFF            = 0x08,
    CIN_NOTE_ON             = 0x09,
    CIN_POLY_KEY_PRESSURE   = 0x0A,
    CIN_CONTROL_CHANGE      = 0x0B,
    CIN_PROGRAM_CHANGE      = 0x0C,
    CIN_CHANNEL_PRESSURE    = 0x0D,
    CIN_PITCH_BEND          = 0x0E,
    CIN_SINGLE_BYTE         = 0x0F,
} midi_usb_cin_t;

#define CIN_SYS_COM_1 0x05          /* System Common 1 byte (Tune Request) */

typedef uint32_t midi_usb_packet_t;

/**
 * @brief Stato di un cable ( == porta )
 *
 * - sysex
 *      flag che segnala che è in corso la codifica
 *      di un sys ex
 *
 * - sysex_data
 *      buffer per la pacchettizzazione dei byte di
 *      dato del sysex
 *
 * - sysex_count
 *      indica quanti byte sono stati accumulati
 *      nel buffer
 */
typedef struct {

    // Sys Ex
    bool    sysex;
    uint8_t sysex_data[3];
    uint8_t sysex_count;

} midi_usb_cable_data_t;

/**
 * @brief Contesto per la codifica dei sys ex
 *
 * - cn
 *      cable number attualmente settato
 *
 * - cin
 *      identificativo dell'ultimo messaggio
 *      usb midi decodificato (not used)
 */
typedef struct {

    // Midi USB
    uint8_t cn;
    uint8_t cin;

    midi_usb_cable_data_t cable[ USB_MIDI_MAX_CABLES ];

} midi_usb_stream_ctx_t;

/**
 * @brief Decodifica MIDI di un pacchetto USB MIDI
 *
 * @param ctx               puntatore al contesto per la codifica usb midi
 * @param midi_packet       pacchetto codificato USB MIDI
 * @param midi_raw_data     puntatore al buffer su cui depositare i byte decodificati
 *
 * @return numero di byte decodificati
 */
uint8_t midi_usb_decode_packet( midi_usb_stream_ctx_t* ctx, midi_usb_packet_t midi_packet, uint8_t* midi_raw_data );

/**
 * @brief Codifica USB MIDI di un messaggio MIDI
 *
 * @param ctx               puntatore al contesto per la codifica usb midi
 * @param midi_raw_data     puntatore al buffer contente i byte mid da codificare
 * @param midi_raw_length   quantità di dati da codificare
 * @param midi_packet       puntatore ai dati codificati
 *
 * @return numero di byte codificati
 *
 * @note
 * - Il messaggio MIDI da codificare deve essere completo, non supporta
 *   running status.
 * - Non può essere utilizzata per codificare sistemi esclusivi
 */
uint8_t midi_usb_encode_message( midi_usb_stream_ctx_t* ctx, uint8_t* midi_raw_data, uint8_t midi_raw_length, midi_usb_packet_t* midi_packet );

/**
 * Inizializzazione contesto di codifica usb midi per sys ex
 *
 * @param ctx           puntatore al contesto per la codifica usb midi
 */
void midi_usb_init_ctx( midi_usb_stream_ctx_t* ctx );

/**
 * Setta il numero di cable corrente nel contesto usb midi
 *
 * @param ctx           puntatore al contesto per la codifica usb midi
 * @param cn            cable number
 */
static inline void midi_usb_set_cable( midi_usb_stream_ctx_t* ctx, uint8_t cn )
{
    if ( !ctx || cn >= USB_MIDI_MAX_CABLES )
        return;

    ctx->cn = cn;
}

static inline uint8_t midi_usb_get_cable( midi_usb_stream_ctx_t* ctx )
{
    if ( !ctx )
        return 0xFF;

    return ctx->cn;
}

/**
 * Codifica USB MIDI di dati sys ex
 *
 * @param ctx           puntatore al contesto per la codifica usb midi
 * @param cn            cable number
 * @param byte          dato sys ex
 * @param midi_packet   puntatore al pacchetto usb midi
 *
 * @return numero di byte codificati
 */
uint8_t midi_usb_encode_sysex( midi_usb_stream_ctx_t* ctx, uint8_t byte, midi_usb_packet_t* midi_packet );

#ifdef __cplusplus
}
#endif

#endif // MIDI_USB_STREAM_H
