# qn-enh-sdk
一些图像增强的ffmpeg filter

## 编译说明
> 先编译 proj-qnfilter 和 proj-qndeblock-filter
1. 进入 ->proj-qnfilter

*注意，我是windows*
```
-> cd proj-qnfilter
-> mkdir build
-> cd build
->  cmake -G"Unix Makefiles" ../
-> make & make install
```
###### proj-qndeblock-filter 同样操作即可

2. 然后执行 to_ffmpeg.py
 
*这里要注意的是，ffmpeg 要先 configure，我的ffmpeg configure 命令是 ./scripts/myconf.bat ，仅供参考*
```
(./qn-enh-sdk)-> python3 to_ffmpeg.py 
```

3. copy 完以后是去编译 ffmpeg

## ffmpeg 命令
sample 如下：
```
ffmpeg -i fei.mp4 -vf qnfilter=enhtype=lowlight_enh:w=0.9 -t 300 -acodec copy -y -vcodec h264 fei-light.mp4
ffmpeg-h -i 48.mp4 -vf qndeblock=modelurl="../../nir10_best.onnx" -vcodec h264 a.mp4
可以混合使用：
ffmpeg -i fei.mp4 -vcodec h264 -vf qnfilter=enhtype=lowlight_enh:w=0.9,qndeblock=modelurl="../../nir10_best.onnx" fei-f.mp4
去雾：
ffmpeg -i haze.mp4 -vcodec h264 -vf qnfilter=enhtype=dehaze:w=0.8 haze-de.mp4
```

## filter 参数说明
当前版本仅支持两个 enhtyp：*lowlight_enh* 和 *dehaze*，参数仅有一个：*w*，表示强度，也可以不设置 w，使用默认效果

## 效果展示
*lowlight_enh 应该实用性还不错，可以比较显著的增大对比度改善视觉评价*

> ./demo 中有两个视频，xgm-low.mp4 是视频增强后的效果-

## 特别注意事项
./scripts/myconf.bat 中的 CUDA_PATH 环境变量中如果有空格，则需要去掉该变量相关 -L 和 -I 选项，直接去 ffmpeg_dir/ffbuild/config.mak 中做相应修改
重点修改项是：EXTRALIBS 和 CFLAGS，相应的 -I、-L 和 -l
