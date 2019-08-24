# Parameters for conversion
ffmpeg_par_audio = -i "%ifname%" -y -vn -ar 44100 -ac 2 -f mp3 -ab 128k "%ofname%"
ffmpeg_par_video = -analyzeduration 2147483647 -probesize 2147483647 -i "%ifname%" -y -vf "scale=-2:'min(480,ih)'" -preset slow -crf 20 -b:a 128k "%ofname%"
ffmpeg_par_image = -i "%ifname%" -y -q:v 2 -vf scale='min(1920,iw)':-2 "%ofname%"
magick_par_image = convert "%ifname%" -resize 1920 "%ofname%"

# Main
ignore_2 = 0 # Set to 1 to ignore files *_2.jpg, *_3.jpg, *_4.jpg...

# Remove [to cut] tags from filenames containing [cut] tags (processed before shortening filenames)
strip_tocut = 1 

# File length (processes even ignored files to prevent filesystem problems)
shorten_filenames_to = 100 # Comment or set to 0 to disable file shortener

# process_links=0: Do not process links
# process_links=1: Fix links (checked before remove_ext)
# process_links=2: Replace links with linked files (checked before remove_ext)
process_links = 0

# save_exif=0: Do not save exif when processing (can process faster)
# save_exif=1: Save exif when processing
save_exif = 1

# When renaming main file, also rename XMP
rename_xmp = 1

# File extensions to delete (checked after process_links)
remove_ext = ".ithmb"
remove_ext = ".videocache"
remove_ext = ".dat"
remove_ext = ".thm"
remove_ext = ".tmp"
remove_ext = ".peak"

# File masks to ignore (checked after remove_ext)
ignore_match = "480P*"

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
