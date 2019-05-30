# BatchCompress
Recursively compress video and jpeg files in folder using ffmpeg

Functions:
- Resize images to specified width, while preserving aspect ration, without upscaling (default to 1920 pixels). Supports all image formats that ImageMagick supports, for example: JPG, JPEG, BMP, CR2, CRW, ARW, MRW, NEF, ORF, RAF, X3F
- Compress video to MP4 (default to 20 constant rate factor, 128kbit audio). Supports all video formats that FFMPEG supports, for example: VOB, WMV, AVI, FLV, MP4, ASF, MOV, 3GP, M4V, 3GB
- Compress audio to MP3 (default to 44.1 KHz, 128kbit). Supports all audio formats that FFMPEG supports, for example: WAV, MP3
- Delete files that are not needed (by default extensions: ITHMB, VIDEOCACHE, DAT, THM, TMP, PEAK)
- If compressed / resized file is larger than original, source file is preserved and "-noconv" is added to its name. This allows for faster processing if repeated, by not to trying to compress these files again
- Can preserve EXIF of an image if needed during resizing (to do that set save_exif=1 in config)
- Can skip resizing images that end with "_2", "_3" and so on (to do that set ignore_2=1 in config)
- Can replace links with linked files if needed (to do that set process_links=2 in config)

If there is a BatchCompress.pl configuration file in the running folder, will use this config. Else will use config situated in the same folder, where BatchCompress.exe is.
