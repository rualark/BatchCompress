# Parameters for conversion
ffmpeg_par_audio = "-y -vn -ar 44100 -ac 2 -f mp3 -ab 128k"
ffmpeg_par_video = "-y -preset slow -crf 20 -b:a 128k"
ffmpeg_par_image = "-y -q:v 2 -vf scale='min(1920,iw)':-2"
magick_par_image = "-resize 1920"

# Main
#ignore_2 = 1 # Set to 1 to ignore files *_2.jpg, *_3.jpg, *_4.jpg...

# Enable modules
# process_links=0: Do not process links
# process_links=1: Fix links
# process_links=2: Replace links with linked files
process_links = 2
# save_exif=0: Do not save exif when processing (can process faster)
# save_exif=1: Save exif when processing
save_exif = 1

# File extensions to convert to jpeg
image_ext = ".cr2"
image_ext = ".crw"
image_ext = ".arw"
image_ext = ".mrw"
image_ext = ".nef"
image_ext = ".orf"
image_ext = ".raf"
image_ext = ".x3f"
image_ext = ".bmp"

# File extensions to convert to mp4
video_ext = ".vob"
video_ext = ".wmv"
video_ext = ".avi"
video_ext = ".flv"
video_ext = ".mp4"
video_ext = ".asf"
video_ext = ".mov"
video_ext = ".3gp"
video_ext = ".m4v"
video_ext = ".3gp"

# File extensions to convert to MP3
audio_ext = ".wav"
audio_ext = ".mp3"

# File extensions to delete
remove_ext = ".ithmb"
remove_ext = ".videocache"
remove_ext = ".dat"
remove_ext = ".thm"
remove_ext = ".tmp"
remove_ext = ".peak"
