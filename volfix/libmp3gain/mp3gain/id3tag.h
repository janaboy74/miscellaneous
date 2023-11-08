#ifndef ID3TAG_H
#define ID3TAG_H

int ReadMP3GainID3Tag(wchar_t *filename, struct MP3GainTagInfo *info);

int WriteMP3GainID3Tag(wchar_t *filename, struct MP3GainTagInfo *info, int saveTimeStamp);

int RemoveMP3GainID3Tag(wchar_t *filename, int saveTimeStamp);

#endif
