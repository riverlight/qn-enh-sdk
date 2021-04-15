# -*- encoding: utf-8 -*-
from pathlib import Path
import os


g_ffmpeg_dir = "D:\\workroom\\tools\\media\\ffmpeg-4.3.1"
g_qnfilter_dir = ("./src/libavfilter", "proj-qnfilter")
g_qndeblock_dir = ("./src/libavfilter", "proj-qndeblock-filter")


def copy_filter(filter_dir, sdk_dir):
    if not Path(filter_dir).is_dir():
        print("qnfilter dir doesn't exist")
        exit(-2)
    print("check dir done")
    os.system("mkdir {}".format(os.path.join(g_ffmpeg_dir, sdk_dir)))
    print('start copy...')
    cmd = 'cp -r {} {}'.format(filter_dir, g_ffmpeg_dir)
    print(cmd)
    os.system(cmd)
    list_subdir = ['include', 'lib']
    for subdir in list_subdir:
        cmd = 'cp -r {} {}'.format(os.path.join(sdk_dir, subdir), os.path.join(g_ffmpeg_dir, sdk_dir))
        os.system(cmd)
    print('copy done')

def main():
    print("start check dir...")
    copy_filter(g_qnfilter_dir[0], g_qnfilter_dir[1])
    copy_filter(g_qndeblock_dir[0], g_qndeblock_dir[1])
    

if __name__ == "__main__":
    main()
