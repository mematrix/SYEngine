#ifndef _OPUS_HELP_H
#define _OPUS_HELP_H

int opus_packet_get_samples_per_frame(const unsigned char *data, int Fs);
int opus_packet_get_nb_channels(const unsigned char *data);
int opus_packet_get_nb_frames(const unsigned char packet[], int len);
int opus_packet_get_nb_samples(const unsigned char packet[], int len, int Fs);

#endif //_OPUS_HELP_H