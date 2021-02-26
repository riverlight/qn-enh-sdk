# -*- encoding: utf-8 -*-
from pathlib import Path
import os


g_ffmpeg_dir = "D:\\workroom\\tools\\media\\ffmpeg-4.3.1"
g_qnfilter_dir = "./src/libavfilter"
sdk_dir = "proj-qnfilter"

def main():
    print("start check dir...")
    if not Path(g_ffmpeg_dir).is_dir():
        print("ffmpeg dir doesn't exist")
        exit(-1)
    if not Path(g_qnfilter_dir).is_dir():
        print("qnfilter dir doesn't exist")
        exit(-2)
    print("check dir done")
    os.system("mkdir {}".format(os.path.join(g_ffmpeg_dir, sdk_dir)))
    print('start copy...')
    cmd = 'cp -r {} {}'.format(g_qnfilter_dir, g_ffmpeg_dir)
    print(cmd)
    os.system(cmd)
    list_subdir = ['include', 'lib']
    for subdir in list_subdir:
        cmd = 'cp -r {} {}'.format(os.path.join(sdk_dir, subdir), os.path.join(g_ffmpeg_dir, sdk_dir))
        os.system(cmd)
    print('copy done')

if __name__ == "__main__":
    main()
