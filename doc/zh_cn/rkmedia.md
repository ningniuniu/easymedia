***
rkmedia 封装库
=============

简介
----

rkmedia为了使多媒体相关开发更简单而做，将比较偏底层一些的代码或功能抽象简化为更简易的接口。

目前包含了视频硬件编解码接口，音频软件编解码接口，媒体格式封装解封装接口等。

视频硬件编码
----------

- 编译

    确保外部设置-DRKMPP=ON

- 范例：[mpp_enc_test.cc](../../rkmpp/test/mpp_enc_test.cc)

    使用命令查看使用方法：./rkmpp_test -? （在某些极度裁剪的平台上，可能默认生成的固件里没有
    此可执行bin，需要到pc上生成的路径手动push到板端）。

- 接口说明及流程

    * rkmedia::REFLECTOR(Encoder)::DumpFactories() ：列出当前编入的编码模块
    * rkmedia::REFLECTOR(Encoder)::Create\<rkmedia::VideoEncoder\> ：创建视频编码器
    实例，参数为上述的DumpFactories中列出的一个模块对应的字符串
    * InitConfig：初始化编码器，参数为所需编码算法对应的设置系数
    * Process：执行编码，参数为原始未压缩图像数据buffer、压缩图像输出的buffer以及额外的输出
    buffer（如需要h264中的mv数据，在此buffer输出）  
    注意的是，调用此函数前，需要给所有buffer SetValidSize表明buffer可访问的长度空间。
    最后输出buffer的数据长度以GetValidSize体现。
