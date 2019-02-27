***
rkmedia 封装库
=============

简介
----

rkmedia为了使多媒体相关开发更简单而做，将比较偏底层一些的代码或功能抽象简化为更简易的接口。

目前包含了视频硬件编解码接口，媒体格式封装解封装接口，音频软件编解码接口，音频采集输出播放接口等。

视频硬件编码
----------

- 编译

    确保外部设置-DRKMPP=ON

- 范例：[mpp_enc_test.cc](../../frameworks/media/rkmpp/test/mpp_enc_test.cc)

    使用命令查看使用方法：./rkmpp_test -? （可能默认生成的固件里没有此可执行bin，需要到pc上生成的路径手动push到板端）。

- 接口及范例流程说明

    * (可不调用)rkmedia::REFLECTOR(Encoder)::DumpFactories() ：列出当前编入的编码模块
    * rkmedia::REFLECTOR(Encoder)::Create\<rkmedia::VideoEncoder\> ：创建视频编码器实例，参数为上述的DumpFactories中列出的一个模块对应的字符串
    * InitConfig：初始化编码器，参数为所需编码算法对应的设置系数
    * GetExtraData：获取参数信息数据，h264的pps和sps数据在此返回的buffer里
    * Process：执行编码，参数为原始未压缩图像数据buffer、压缩图像输出的buffer以及额外的输出buffer（如需要h264中的mv数据，在此buffer输出）  
    注意的是，调用此函数前，需要给所有buffer SetValidSize表明buffer可访问的长度空间。最后输出buffer的数据长度以GetValidSize体现。

媒体格式解封装
------------

- 编译

    确保外部设置-D\<FORMAT\>=ON -D\<FORMAT\>_DEMUXER=ON  
    比如ogg音频解封装解码oggvorbis，就是-DOGGVORBIS=ON -DOGGVORBIS_DEMUXER=ON

- 范例：[ogg_decode_test.cc](../../frameworks/media/ogg/test/ogg_decode_test.cc)

    使用命令查看使用方法：./rkogg_decode_test -? （可能默认生成的固件里没有此可执行bin，需要到pc上生成的路径手动push到板端）。

- 接口及范例流程说明

    * (可不调用)rkmedia::REFLECTOR(Demuxer)::DumpFactories()：列出当前编入的格式解封装模块
    * rkmedia::REFLECTOR(Demuxer)::Create\<rkmedia::Demuxer\>：创建格式解封装实例，参数为上述的DumpFactories中列出的一个模块对应的字符串和其他一些设置项
    * IncludeDecoder()：判断是否此解封装模块已包含了解码功能。范例中oggvorbis比较特殊，包含了解码功能，否则需要按章节[音频解码](#音频解码)所述创建解码器实例为后续解封装出来的对应数据做解码
    * Init(Stream *input, MediaConfig *out_cfg)：设置输入流和获取音频参数
    * Read：读取一次数据

音频播放输出
----------

> RK目前仅支持alsa，后续可能支持pulseaudio

- 编译

    确保外部设置-DALSA_PLAYBACK=ON

- 范例：[ogg_decode_test.cc](../../frameworks/media/ogg/test/ogg_decode_test.cc)

    使用命令：./rkogg_decode_test -i test.ogg -o alsa:default

- 接口及范例流程说明

    * (可不调用)rkmedia::REFLECTOR(Stream)::DumpFactories()：列出当前编入输入输出模块，stream的操作方法参数与c的FILE类似
    * rkmedia::REFLECTOR(Stream)::Create\<rkmedia::Stream\>：创建音频播放输出流实例，参数为上述的DumpFactories中列出的一个模块对应的字符串和设置打开设备的参数。参数必须如范例中所示，包含device=\*\\nformat=\*\\nchannels=\*\\nsample_rate=\*\\n
    * Write：写入一次数据，参数为buffer对应的sample size和sample numbers
    * Close：关闭输出流
