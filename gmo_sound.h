#ifndef _GMO_SOUND_H_

#include "al.h"
#include "alc.h"
#include "gmo_types.h"
#include "gmo_math.h"
//check this for PCM data
typedef struct 
{
     int32 chunkID;//RIFF
     int32 chunkSize;//4 + 24 + 8 + MN_cN_s + 0 or 1
     int32 waveID;//WAVE
     int32 subChunk1ID;//FMT
     int32 subChunk1Size;//16
     int16 audioFormat;//wave_format_pcm
     int16 numChannels;//N_c
     int32 sampleRate;//samples per sec
     int32 byteRate;//bytes per sec
     int16 blockAlign;//
     int16 bitsPerSample;//     
     int32 subChunk2ID;//data 
     int32 subChunk2Size;//if this is odd, we alocate 1 byte?

} Wav_Header;


typedef struct
{
     int32 size;
     int16 blockAlign;
     int16 bitsPerSample;
     void *data;
} Wav_Buffer;


void list_al_audio_devices(const ALCchar *devices);
void load_wav(char *file_name, ALvoid **wav_data, Wav_Header *wav_header_in, ALsizei *wav_size);
ALenum get_al_format(Wav_Header wav_header);
ALuint al_buffer_from_wav(char *file_name);
void al_make_source(ALCcontext *al_context, ALuint *al_source, ALuint al_buffer, vec3 position, vec3 velocity, real32 pitch, real32 gain, ALboolean looping);

#define _GMO_SOUND_H_
#endif
