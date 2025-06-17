import os
import glob
import os.path
from PIL import Image


def CreateDir(strDir):
    """
    若文件夹不存在则创建
    :param strDir: 文件路径
    :return: 无
    """
    if not os.path.exists(strDir):
        os.makedirs(strDir)


def ConvertJpg(strJpgFile, strSaveDir, width_ratio=0.4, height_ratio=0.4):
    """
    修改图片文件大小 strJpgFile 并保存至 strSaveDir
    :param strJpgFile:  jpg文件路径
    :param strSaveDir:  转换后文件存储路径
    :param width:       图像宽度
    :param height:      图像高度
    :return:            无
    """
    img_src = Image.open(strJpgFile)
    # 注意选择缩放算法
    # img_dst = img_src.resize((width, height), Image.BILINEAR)  # 双线性插值(放大时图像较为模糊，细节损失较严重)
    # img_dst = img_src.resize((width, height), Image.BICUBIC) # 双三次插值(二维空间中最常用的插值方法)
    img_dst = img_src.resize((int(img_src.width*width_ratio), int(img_src.height*height_ratio)), Image.BICUBIC) # 得到的图像在抗锯齿和保留锐利边缘的效果较好
    img_dst.save(os.path.join(strSaveDir, os.path.basename(strJpgFile)))


def BatchModifyJpgSize(fileDir, saveDir):
    """
    查找给定路径下图片文件，并修改其大小
    :param fileDir: 文件路径
    :param saveDir: 目标存储路径
    :return:        无
    """
    for jpgFile in glob.glob(fileDir):
        ConvertJpg(jpgFile, saveDir)


def main():
    # 执行代码
    str_src_file = r'*.png'
    str_output_dir = r'output'
    CreateDir(str_output_dir)
    BatchModifyJpgSize(str_src_file, str_output_dir)
    print('job done.')


if __name__ == '__main__':
    main()

