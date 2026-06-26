"""
使用 Python struct 库生成 Minecraft 风格的 16x16 BMP 贴图
每个方块类型一种贴图，24-bit 色彩
"""
import struct
import os

# ===========================
# 配置
# ===========================
TEX_SIZE = 16  # 16x16 像素
OUTPUT_DIR = r"e:\360MoveData\Users\Administrator\Desktop\3d-minecraft\textures"

BLOCK_TYPES = [
    "grass",    # 草方块
    "dirt",     # 泥土
    "stone",    # 石头
    "wood",     # 木头
    "leaves",   # 树叶
    "sand",     # 沙子
    "water",    # 水
    "coal",     # 煤矿
    "iron",     # 铁矿
    "gold",     # 金矿
    "diamond",  # 钻石矿
]


def make_bmp(pixels_24bit, filename):
    """
    使用 struct 库写入 24-bit BMP 文件
    pixels_24bit: list of (R, G, B) tuples, row-major (top to bottom)
    """
    width = TEX_SIZE
    height = TEX_SIZE
    
    # BMP 行必须是 4 字节对齐
    row_size = (width * 3 + 3) & ~3
    padding = row_size - width * 3
    
    pixel_data = b""
    # BMP 存储是从下到上的行顺序
    for y in range(height - 1, -1, -1):
        row_start = y * width
        for x in range(width):
            r, g, b = pixels_24bit[row_start + x]
            pixel_data += struct.pack("<BBB", b, g, r)  # BMP 是 BGR 顺序
        pixel_data += b"\x00" * padding  # 填充字节
    
    bfSize = 54 + len(pixel_data)  # 14 + 40 + pixel_data
    
    # BITMAPFILEHEADER (14 bytes)
    bmp_header = struct.pack(
        "<HIHHI",
        0x4D42,     # bfType: 'BM'
        bfSize,     # bfSize: 文件总大小
        0,          # bfReserved1
        0,          # bfReserved2
        54          # bfOffBits: 像素数据偏移
    )
    
    # BITMAPINFOHEADER (40 bytes)
    bmp_info = struct.pack(
        "<IiiHHIIiiII",
        40,              # biSize
        width,           # biWidth
        height,          # biHeight
        1,               # biPlanes
        24,              # biBitCount
        0,               # biCompression (BI_RGB)
        len(pixel_data), # biSizeImage
        2835,            # biXPelsPerMeter (72 DPI)
        2835,            # biYPelsPerMeter (72 DPI)
        0,               # biClrUsed
        0                # biClrImportant
    )
    
    with open(filename, "wb") as f:
        f.write(bmp_header)
        f.write(bmp_info)
        f.write(pixel_data)
    
    print(f"  已生成: {filename} ({width}x{height}, {len(pixel_data)} bytes pixel data)")


def noise_hash(x, y, seed=42):
    """简单的随机哈希函数，用于生成噪点"""
    h = seed
    h ^= (x * 374761393 + y * 668265263) & 0xFFFFFFFF
    h = ((h << 13) ^ h) & 0xFFFFFFFF
    h = (h * 1274126177) & 0xFFFFFFFF
    return (h & 0xFF) / 255.0


def blend(a, b, t):
    """颜色混合"""
    return (
        int(a[0] + (b[0] - a[0]) * t),
        int(a[1] + (b[1] - a[1]) * t),
        int(a[2] + (b[2] - a[2]) * t),
    )


def lerp_color(a, b, t):
    return blend(a, b, t)


def darken(c, amount):
    return (
        max(0, int(c[0] * amount)),
        max(0, int(c[1] * amount)),
        max(0, int(c[2] * amount)),
    )


def lighten(c, amount):
    return (
        min(255, int(c[0] + (255 - c[0]) * amount)),
        min(255, int(c[1] + (255 - c[1]) * amount)),
        min(255, int(c[2] + (255 - c[2]) * amount)),
    )


# ===========================
# 各贴图生成函数
# ===========================

def gen_grass():
    """草方块：顶部绿色带草纹，侧面绿色渐变+泥土带"""
    pixels = []
    top_color = (85, 180, 50)
    side_color = (100, 160, 45)
    dirt_color = (130, 80, 30)
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            # 顶部 4 行是草的"顶面"视角风格
            if y < 4:
                base = top_color
                n = noise_hash(x, y, 10)
                c = darken(base, 0.85 + n * 0.3)
                # 添加草纹斑点
                if noise_hash(x + 7, y + 3, 20) > 0.65:
                    c = lighten(c, 0.25)
                pixels.append(c)
            # 中间是草侧面的绿色部分
            elif y < 12:
                base = side_color
                n = noise_hash(x, y, 30)
                c = darken(base, 0.8 + n * 0.4)
                # 竖纹
                if x % 3 == 0:
                    c = darken(c, 0.9)
                pixels.append(c)
            # 底部是泥土带
            else:
                base = dirt_color
                n = noise_hash(x, y, 40)
                c = darken(base, 0.78 + n * 0.44)
                # 横纹
                if y % 2 == 0:
                    c = darken(c, 0.92)
                pixels.append(c)
    return pixels


def gen_dirt():
    """泥土：棕色带颗粒纹理"""
    pixels = []
    base_color = (130, 85, 35)
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            n1 = noise_hash(x, y, 50)
            n2 = noise_hash(x + 5, y + 5, 55)
            c = blend(base_color, darken(base_color, 0.6), n1 * 0.5)
            # 小石子斑点
            if n2 > 0.82:
                c = darken(c, 0.65)
            elif n2 > 0.7:
                c = lighten(c, 0.15)
            # 细微噪点
            n3 = noise_hash(x + 3, y + 7, 60)
            c = blend(c, darken(c, 0.8), n3 * 0.3)
            pixels.append(c)
    return pixels


def gen_stone():
    """石头：灰色带裂纹纹理"""
    pixels = []
    base_color = (140, 140, 140)
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            n = noise_hash(x, y, 70)
            d = noise_hash(x + 4, y + 4, 75)
            
            # 基础灰阶变化
            c = blend(base_color, darken(base_color, 0.7), n * 0.6)
            
            # 裂纹：在某些位置加深
            crack = noise_hash(x * 2, y + 8, 80)
            if crack > 0.75:
                c = darken(c, 0.7)
            elif crack > 0.6:
                c = lighten(c, 0.15)
            
            # 细微纹理
            c = blend(c, darken(c, 0.85), d * 0.4)
            pixels.append(c)
    return pixels


def gen_wood():
    """木头：棕色带年轮/竖纹"""
    pixels = []
    base_color = (150, 100, 40)
    dark_color = (100, 60, 20)
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            # 年轮效果：用正弦波 + 噪点
            ring = abs(x - 8) / 8.0  # 0到1，边缘深
            ring = ring ** 1.5
            
            n = noise_hash(x, y, 90)
            c = blend(dark_color, base_color, ring * 0.8 + n * 0.2)
            
            # 竖纹
            if y % 3 == 0:
                c = darken(c, 0.9)
            
            # 树结
            if 3 <= x <= 5 and 5 <= y <= 7:
                c = darken(c, 0.65)
            if 11 <= x <= 13 and 9 <= y <= 11:
                c = darken(c, 0.7)
            
            pixels.append(c)
    return pixels


def gen_leaves():
    """树叶：绿色带叶脉纹理，有透明感的间隙"""
    pixels = []
    base_color = (40, 130, 30)
    light_green = (80, 180, 50)
    dark_green = (15, 80, 10)
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            n1 = noise_hash(x, y, 100)
            n2 = noise_hash(x + 6, y + 6, 105)
            
            # 基础绿色
            c = blend(dark_green, light_green, n1 * 0.7 + 0.15)
            
            # 叶脉网格
            if (x + y) % 5 == 0 or abs(x - y) % 5 == 0:
                c = darken(c, 0.75)
            
            # 间隙（模拟树叶缝隙）
            if n2 > 0.78:
                c = lighten(c, 0.5)  # 透亮的间隙
            elif n2 > 0.88:
                c = (30, 160, 50)  # 亮绿光斑
            
            pixels.append(c)
    return pixels


def gen_sand():
    """沙子：黄色/米色带颗粒纹理"""
    pixels = []
    base_color = (225, 215, 160)
    dark_sand = (200, 180, 120)
    light_sand = (245, 235, 190)
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            n1 = noise_hash(x, y, 110)
            n2 = noise_hash(x + 3, y + 3, 115)
            
            # 沙粒渐变
            c = blend(dark_sand, light_sand, n1)
            
            # 细小沙粒高光
            if n2 > 0.8:
                c = lighten(c, 0.3)
            elif n2 < 0.2:
                c = darken(c, 0.85)
            
            # 微纹理
            n3 = noise_hash(x + 7, y + 1, 120)
            c = blend(c, darken(c, 0.9), n3 * 0.35)
            pixels.append(c)
    return pixels


def gen_water():
    """水：蓝色带波纹"""
    pixels = []
    base_color = (30, 100, 200)
    light_water = (60, 160, 240)
    dark_water = (10, 50, 140)
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            # 波纹
            wave = (noise_hash(x, y, 130) + noise_hash(x + 4, y + 4, 135)) / 2.0
            
            c = blend(dark_water, light_water, wave * 0.8 + 0.1)
            
            # 横向波纹带
            stripe = abs((y - 8) / 8.0)
            c = blend(c, lighten(c, 0.3), (1.0 - stripe) * 0.2)
            
            # 高光点
            if wave > 0.82:
                c = lighten(c, 0.4)
            
            pixels.append(c)
    return pixels


def gen_coal():
    """煤矿：深灰色石头纹理上点缀黑斑"""
    pixels = gen_stone()  # 先有石头底纹
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            idx = y * TEX_SIZE + x
            n = noise_hash(x, y, 140)
            # 黑煤斑点
            if n > 0.5:
                coal_dark = (25, 25, 25)
                c = blend(pixels[idx], coal_dark, (n - 0.5) * 2.0)
                pixels[idx] = c
            # 煤块高光边缘
            n2 = noise_hash(x + 8, y + 8, 145)
            if 0.72 < n < 0.78:
                pixels[idx] = lighten(pixels[idx], 0.3)
    return pixels


def gen_iron():
    """铁矿：灰色石头纹理上点缀铁锈斑点"""
    pixels = gen_stone()
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            idx = y * TEX_SIZE + x
            n = noise_hash(x, y, 150)
            # 铁锈色斑点
            if n > 0.55:
                iron_rust = (200, 160, 110)
                t = (n - 0.55) / 0.45
                c = blend(pixels[idx], iron_rust, t * 0.7)
                pixels[idx] = c
            # 银色亮斑
            if 0.48 < n < 0.55:
                pixels[idx] = lighten(pixels[idx], 0.4)
    return pixels


def gen_gold():
    """金矿：石头纹理上点缀金色斑点"""
    pixels = gen_stone()
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            idx = y * TEX_SIZE + x
            n = noise_hash(x, y, 160)
            # 金色斑点
            if n > 0.55:
                gold_color = (255, 200, 40)
                t = (n - 0.55) / 0.45
                c = blend(pixels[idx], gold_color, t * 0.85)
                pixels[idx] = c
            # 闪光点
            if 0.45 < n < 0.52:
                pixels[idx] = lighten(pixels[idx], 0.5)
    return pixels


def gen_diamond():
    """钻石矿：石头纹理上点缀青蓝色钻石斑点"""
    pixels = gen_stone()
    
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            idx = y * TEX_SIZE + x
            n = noise_hash(x, y, 170)
            # 钻石斑点 - 青蓝色
            if n > 0.6:
                diamond_color = (40, 200, 200)
                t = (n - 0.6) / 0.4
                c = blend(pixels[idx], diamond_color, t * 0.8)
                pixels[idx] = c
            # 高亮闪光
            if 0.5 < n < 0.58:
                pixels[idx] = lighten(pixels[idx], 0.45)
    return pixels


# ===========================
# 主程序
# ===========================
def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    generators = {
        "grass": gen_grass,
        "dirt": gen_dirt,
        "stone": gen_stone,
        "wood": gen_wood,
        "leaves": gen_leaves,
        "sand": gen_sand,
        "water": gen_water,
        "coal": gen_coal,
        "iron": gen_iron,
        "gold": gen_gold,
        "diamond": gen_diamond,
    }
    
    print("=" * 50)
    print("  使用 Python struct 库生成 Minecraft 风格贴图")
    print("=" * 50)
    
    for name, gen_func in generators.items():
        pixels = gen_func()
        filepath = os.path.join(OUTPUT_DIR, f"{name}.bmp")
        make_bmp(pixels, filepath)
    
    print("=" * 50)
    print(f"  共生成 {len(generators)} 张贴图到: {OUTPUT_DIR}")
    print("=" * 50)


if __name__ == "__main__":
    main()
