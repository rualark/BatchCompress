# Parameters for conversion
ffmpeg_par_audio = "-y -vn -ar 44100 -ac 2 -f mp3 -ab 128k"
ffmpeg_par_video = "-y -preset slow -crf 20 -b:a 128k"
ffmpeg_par_image = "-y -q:v 2 -vf scale='min(1920,iw)':-2"
magick_par_image = "-resize 1920"

# File extensions to convert to jpeg
image_ext = ".cr2"
image_ext = ".crw"
image_ext = ".arw"
image_ext = ".mrw"
image_ext = ".nef"
image_ext = ".orf"
image_ext = ".raf"
image_ext = ".x3f"

# File extensions to convert to mp4
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

# File extensions to delete
remove_ext = ".ithmb"
remove_ext = ".videocache"
remove_ext = ".dat"
remove_ext = ".thm"
