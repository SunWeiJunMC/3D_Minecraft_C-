#include <GL/glut.h>
#include <bits/stdc++.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <windows.h>

using namespace std;

// ===========================
// ��������������Ҫ��
// ===========================
const int WORLD_SIZE_X = 1024;
const int WORLD_SIZE_Y = 128;
const int WORLD_SIZE_Z = 1024;

const int WINDOW_W = 1920;
const int WINDOW_H = 1080;

// Chunk
const int CHUNK_SIZE = 16;
const int CHUNK_X = WORLD_SIZE_X / CHUNK_SIZE;
const int CHUNK_Y = WORLD_SIZE_Y / CHUNK_SIZE;
const int CHUNK_Z = WORLD_SIZE_Z / CHUNK_SIZE;
int VIEW_CHUNK_RADIUS = 6;

// UI
const int HOTBAR_SIZE = 9;
const int INVENTORY_SIZE = 36;
const int CRAFTING_GRID_SIZE = 3;
const float PLAYER_RADIUS = 0.3f;
const float PLAYER_HEIGHT = 1.8f;
const float GRAVITY = 20.0f;
const float WALK_SPEED = 4.3f;
const float JUMP_HEIGHT = 1.2f;
const float FIXED_DT = 1.0f / 120.0f;

const int CENTER_X = WINDOW_W / 2;
const int CENTER_Y = WINDOW_H / 2;
const float MOUSE_SENS = 0.0022f;
enum ItemType {
    AIR_ITEM,
    GRASS_BLOCK,
    DIRT_BLOCK,
    STONE_BLOCK,
    WOOD_BLOCK,
    LEAVES_BLOCK,
    SAND_BLOCK,
    WATER_BLOCK,
    COAL_BLOCK,
    IRON_BLOCK,
    GOLD_BLOCK,
    DIAMOND_BLOCK,
    WOODEN_PICKAXE,
    STONE_PICKAXE,
    IRON_PICKAXE,
    GOLDEN_PICKAXE,
    DIAMOND_PICKAXE,
    WOODEN_AXE,
    STONE_AXE,
    IRON_AXE,
    GOLDEN_AXE,
    DIAMOND_AXE,
    WOODEN_SHOVEL,
    STONE_SHOVEL,
    IRON_SHOVEL,
    GOLDEN_SHOVEL,
    DIAMOND_SHOVEL
};

enum BlockType : uint8_t {
    AIR = 0,
    GRASS,
    DIRT,
    STONE,
    WOOD,
    LEAVES,
    SAND,
    WATER,
    COAL,
    IRON,
    GOLD,
    DIAMOND
};

struct Item {
    ItemType type;
    int count;
    Item() : type(AIR_ITEM), count(0) {}
    Item(ItemType t, int c = 1) : type(t), count(c) {}
    bool isEmpty() const { return type == AIR_ITEM || count <= 0; }
};

struct Recipe {
    ItemType result;
    int resultCount;
    ItemType ingredients[CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE];
    Recipe(ItemType res, int resCount, const ItemType* ingr)
        : result(res), resultCount(resCount) {
        for (int i = 0; i < CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE; ++i)
            ingredients[i] = ingr[i];
    }
};

struct Inventory {
    Item items[INVENTORY_SIZE];
    Item hotbar[HOTBAR_SIZE];
    Item craftingGrid[CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE];
    Item craftingResult;
    int selectedHotbarSlot;
    bool showInventory;
    bool showCrafting;

    Inventory() : selectedHotbarSlot(0), showInventory(false), showCrafting(false) {
        for (int i = 0; i < INVENTORY_SIZE; ++i) items[i] = Item();
        // ��ʼ��Ʒ
        hotbar[0] = Item(GRASS_BLOCK, 64);
        hotbar[1] = Item(DIRT_BLOCK, 64);
        hotbar[2] = Item(STONE_BLOCK, 64);
        hotbar[3] = Item(WOOD_BLOCK, 64);
        hotbar[4] = Item(LEAVES_BLOCK, 64);
        hotbar[5] = Item(SAND_BLOCK, 64);
        hotbar[6] = Item(WATER_BLOCK, 64);

        for (int i = 0; i < HOTBAR_SIZE; ++i) if (hotbar[i].isEmpty()) hotbar[i] = Item();
        for (int i = 0; i < CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE; ++i) craftingGrid[i] = Item();
        craftingResult = Item();
    }

    // �ȶѵ���������ٶѵ����������ҿ�λ������MC��
    bool addItem(const Item& inItem) {
        Item item = inItem;
        if (item.isEmpty()) return true;

        auto tryStack = [&](Item* arr, int n) {
            for (int i = 0; i < n; ++i) {
                if (arr[i].type == item.type && arr[i].count > 0 && arr[i].count < 64) {
                    int can = min(64 - arr[i].count, item.count);
                    arr[i].count += can;
                    item.count -= can;
                    if (item.count <= 0) return true;
                }
            }
            return false;
        };

        if (tryStack(hotbar, HOTBAR_SIZE)) return true;
        if (tryStack(items, INVENTORY_SIZE)) return true;

        auto tryEmpty = [&](Item* arr, int n) {
            for (int i = 0; i < n; ++i) {
                if (arr[i].isEmpty()) {
                    int put = min(64, item.count);
                    arr[i] = Item(item.type, put);
                    item.count -= put;
                    if (item.count <= 0) return true;
                }
            }
            return false;
        };

        if (tryEmpty(hotbar, HOTBAR_SIZE)) return true;
        if (tryEmpty(items, INVENTORY_SIZE)) return true;

        return false;
    }

    Item getSelectedItem() const { return hotbar[selectedHotbarSlot]; }

    void checkCraftingRecipes(const vector<Recipe>& recipes) {
        craftingResult = Item();
        for (const auto& recipe : recipes) {
            bool match = true;
            for (int i = 0; i < CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE; ++i) {
                if (craftingGrid[i].type != recipe.ingredients[i]) { match = false; break; }
            }
            if (match) { craftingResult = Item(recipe.result, recipe.resultCount); break; }
        }
    }

    bool craftItem() {
        if (craftingResult.isEmpty()) return false;
        if (!addItem(craftingResult)) return false;

        for (int i = 0; i < CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE; ++i) {
            if (!craftingGrid[i].isEmpty()) {
                craftingGrid[i].count--;
                if (craftingGrid[i].count <= 0) craftingGrid[i] = Item();
            }
        }
        return true;
    }
};

// ===========================
// ����
// ===========================
const char* blockNames[] = {
    "Air","Grass","Dirt","Stone","Wood","Leaves","Sand","Water","Coal","Iron","Gold","Diamond"
};

const char* itemNames[] = {
    "Air","Grass Block","Dirt Block","Stone Block","Wood Block","Leaves Block",
    "Sand Block","Water Block","Coal Block","Iron Block","Gold Block","Diamond Block",
    "Wooden Pickaxe","Stone Pickaxe","Iron Pickaxe","Golden Pickaxe","Diamond Pickaxe",
    "Wooden Axe","Stone Axe","Iron Axe","Golden Axe","Diamond Axe",
    "Wooden Shovel","Stone Shovel","Iron Shovel","Golden Shovel","Diamond Shovel"
};

// ===========================
// ������Value Noise + FBM��
// ===========================
static inline float lerp(float a, float b, float t){ return a + (b - a) * t; }
static inline float fade(float t){ return t * t * t * (t * (t * 6 - 15) + 10); }

struct Noise2D {
    uint32_t seed;
    Noise2D(uint32_t s=1337u): seed(s) {}

    uint32_t hash(int x, int y) const {
        uint32_t h = seed;
        h ^= (uint32_t)x * 374761393u;
        h = (h << 13) ^ h;
        h ^= (uint32_t)y * 668265263u;
        h = (h << 13) ^ h;
        return h * 1274126177u;
    }

    float rand01(int x, int y) const {
        uint32_t h = hash(x,y);
        return (h & 0x00FFFFFF) / float(0x01000000);
    }

    float value(float x, float y) const {
        int x0 = (int)floor(x), y0 = (int)floor(y);
        int x1 = x0 + 1, y1 = y0 + 1;
        float sx = fade(x - x0);
        float sy = fade(y - y0);
        float v00 = rand01(x0,y0);
        float v10 = rand01(x1,y0);
        float v01 = rand01(x0,y1);
        float v11 = rand01(x1,y1);
        float ix0 = lerp(v00, v10, sx);
        float ix1 = lerp(v01, v11, sx);
        return lerp(ix0, ix1, sy);
    }

    float fbm(float x, float y, int oct=5, float lac=2.0f, float gain=0.5f) const {
        float amp=1.0f, freq=1.0f, sum=0.0f, norm=0.0f;
        for(int i=0;i<oct;i++){
            sum += amp * value(x*freq, y*freq);
            norm += amp;
            amp *= gain;
            freq *= lac;
        }
        return sum / max(0.0001f, norm);
    }
};

// ===========================
// Chunk ���磺�� block type��uint8��
// + ֻ���ƿɼ��棨�ھ���AIR�Ż���
// + Chunk ���� display list��������ģʽ��
// ===========================
static inline bool inWorld(int x,int y,int z){
    return x>=0 && x<WORLD_SIZE_X && y>=0 && y<WORLD_SIZE_Y && z>=0 && z<WORLD_SIZE_Z;
}

static inline int chunkIndex(int cx,int cy,int cz){
    return (cy*CHUNK_Z + cz)*CHUNK_X + cx;
}

struct Chunk {
    uint8_t blocks[CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE]; // type only
    bool dirty = true;
    GLuint displayList = 0;
    bool generated = false;

    Chunk(){
        memset(blocks, 0, sizeof(blocks));
    }
    inline uint8_t& at(int lx,int ly,int lz){
        return blocks[(ly*CHUNK_SIZE + lz)*CHUNK_SIZE + lx];
    }
    inline uint8_t atc(int lx,int ly,int lz) const{
        return blocks[(ly*CHUNK_SIZE + lz)*CHUNK_SIZE + lx];
    }
};

vector<Chunk> chunks(CHUNK_X*CHUNK_Y*CHUNK_Z);

// ===========================
// BMP 纹理系统
// ===========================
#ifndef GL_BGR_EXT
#define GL_BGR_EXT 0x80E0
#endif

GLuint blockTextures[13]; // 索引对应 BlockType，0=AIR 不使用
bool texturesLoaded = false;

// 加载 24-bit BMP 文件并生成 OpenGL 纹理
GLuint loadBMP(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        printf("无法打开纹理: %s\n", filepath);
        return 0;
    }

    // 读取 BMP 文件头 (54 bytes)
    unsigned char header[54];
    if (fread(header, 1, 54, f) != 54) {
        printf("BMP 文件头读取失败: %s\n", filepath);
        fclose(f);
        return 0;
    }

    // 验证 BMP 签名
    if (header[0] != 'B' || header[1] != 'M') {
        printf("不是有效的 BMP 文件: %s\n", filepath);
        fclose(f);
        return 0;
    }

    // 从文件头提取信息
    int dataOffset   = *(int*)&header[10];
    int width        = *(int*)&header[18];
    int height       = *(int*)&header[22];
    int bitCount     = *(short*)&header[28];

    if (bitCount != 24) {
        printf("仅支持 24-bit BMP: %s\n", filepath);
        fclose(f);
        return 0;
    }

    // 计算行大小（BMP 行按 4 字节对齐）
    int rowSize = (width * 3 + 3) & ~3;
    int dataSize = rowSize * height;

    unsigned char* data = new unsigned char[dataSize];
    fseek(f, dataOffset, SEEK_SET);
    if (fread(data, 1, dataSize, f) != (size_t)dataSize) {
        printf("BMP 像素数据读取失败: %s\n", filepath);
        delete[] data;
        fclose(f);
        return 0;
    }
    fclose(f);

    // 生成 OpenGL 纹理
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // 设置纹理参数：Minecraft 风格 —— 近邻过滤 + 重复环绕
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // BMP 存储顺序为 BGR 且从底向上，直接上传
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                 GL_BGR_EXT, GL_UNSIGNED_BYTE, data);

    delete[] data;
    printf("纹理加载成功: %s (%dx%d)\n", filepath, width, height);
    return texID;
}

// 纹理文件名映射（BlockType -> 文件名）
const char* textureFiles[] = {
    NULL,           // AIR
    "textures/grass.bmp",
    "textures/dirt.bmp",
    "textures/stone.bmp",
    "textures/wood.bmp",
    "textures/leaves.bmp",
    "textures/sand.bmp",
    "textures/water.bmp",
    "textures/coal.bmp",
    "textures/iron.bmp",
    "textures/gold.bmp",
    "textures/diamond.bmp",
};

void loadAllTextures() {
    // 获取 exe 所在目录，解决工作目录不一致导致的路径问题
    char exePath[MAX_PATH];
    char exeDir[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    strcpy(exeDir, exePath);
    char* lastSlash = strrchr(exeDir, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';

    int texCount = sizeof(textureFiles) / sizeof(textureFiles[0]);
    for (int i = 1; i < texCount; ++i) {
        if (textureFiles[i]) {
            char fullPath[MAX_PATH];
            snprintf(fullPath, MAX_PATH, "%s%s", exeDir, textureFiles[i]);
            blockTextures[i] = loadBMP(fullPath);
        }
    }
    texturesLoaded = true;
    printf("纹理加载完成，exe目录: %s\n", exeDir);
}

Noise2D terrainNoise(20260228u);

// ===========================
// ���
// ===========================
float playerX = WORLD_SIZE_X / 2.0f;
float playerY = 80.0f;
float playerZ = WORLD_SIZE_Z / 2.0f;

float playerRotX = 0.0f; // pitch
float playerRotY = 0.0f; // yaw

float velX=0, velY=0, velZ=0;
bool onGround = false;

Inventory playerInventory;
vector<Recipe> craftingRecipes;

// ����״̬�����ͬʱ��
bool keyDown[256];
bool specialDown[256];

// ����״̬
bool firstMouse = true;
bool lockMouse = true;

// ===========================
// ����/��Ʒ��ɫ
// ��������ԭ������ɫ��
// ===========================
void setBlockColor(BlockType type) {
    switch(type){
        case GRASS:   glColor3f(0.2f, 0.8f, 0.2f); break;
        case DIRT:    glColor3f(0.5f, 0.3f, 0.0f); break;
        case STONE:   glColor3f(0.6f, 0.6f, 0.6f); break;
        case WOOD:    glColor3f(0.6f, 0.3f, 0.0f); break;
        case LEAVES:  glColor3f(0.0f, 0.6f, 0.0f); break;
        case SAND:    glColor3f(0.9f, 0.9f, 0.6f); break;
        case WATER:   glColor4f(0.0f, 0.4f, 0.8f, 0.5f); break;
        case COAL:    glColor3f(0.1f, 0.1f, 0.1f); break;
        case IRON:    glColor3f(0.7f, 0.7f, 0.7f); break;
        case GOLD:    glColor3f(1.0f, 0.8f, 0.0f); break;
        case DIAMOND: glColor3f(0.0f, 0.8f, 0.8f); break;
        default:      glColor3f(1.0f, 1.0f, 1.0f); break;
    }
}

void setItemColor(ItemType type) {
    switch (type) {
        case GRASS_BLOCK:   glColor3f(0.2f, 0.8f, 0.2f); break;
        case DIRT_BLOCK:    glColor3f(0.5f, 0.3f, 0.0f); break;
        case STONE_BLOCK:   glColor3f(0.6f, 0.6f, 0.6f); break;
        case WOOD_BLOCK:    glColor3f(0.6f, 0.3f, 0.0f); break;
        case LEAVES_BLOCK:  glColor3f(0.0f, 0.6f, 0.0f); break;
        case SAND_BLOCK:    glColor3f(0.9f, 0.9f, 0.6f); break;
        case WATER_BLOCK:   glColor4f(0.0f, 0.4f, 0.8f, 0.5f); break;
        case COAL_BLOCK:    glColor3f(0.1f, 0.1f, 0.1f); break;
        case IRON_BLOCK:    glColor3f(0.7f, 0.7f, 0.7f); break;
        case GOLD_BLOCK:    glColor3f(1.0f, 0.8f, 0.0f); break;
        case DIAMOND_BLOCK: glColor3f(0.0f, 0.8f, 0.8f); break;

        case WOODEN_PICKAXE:
        case WOODEN_AXE:
        case WOODEN_SHOVEL: glColor3f(0.6f, 0.3f, 0.0f); break;
        case STONE_PICKAXE:
        case STONE_AXE:
        case STONE_SHOVEL:  glColor3f(0.6f, 0.6f, 0.6f); break;
        case IRON_PICKAXE:
        case IRON_AXE:
        case IRON_SHOVEL:   glColor3f(0.8f, 0.8f, 0.8f); break;
        case GOLDEN_PICKAXE:
        case GOLDEN_AXE:
        case GOLDEN_SHOVEL: glColor3f(1.0f, 0.8f, 0.0f); break;
        case DIAMOND_PICKAXE:
        case DIAMOND_AXE:
        case DIAMOND_SHOVEL: glColor3f(0.0f, 0.8f, 0.8f); break;
        default: glColor3f(1,1,1); break;
    }
}

// ===========================
// ������ʣ�get/set block��chunk����
// ===========================
uint8_t getBlock(int x,int y,int z){
    if(!inWorld(x,y,z)) return AIR;
    int cx = x / CHUNK_SIZE, cy = y / CHUNK_SIZE, cz = z / CHUNK_SIZE;
    int lx = x % CHUNK_SIZE, ly = y % CHUNK_SIZE, lz = z % CHUNK_SIZE;
    Chunk& c = chunks[chunkIndex(cx,cy,cz)];
    return c.atc(lx,ly,lz);
}

void setBlock(int x,int y,int z, uint8_t t){
    if(!inWorld(x,y,z)) return;
    int cx = x / CHUNK_SIZE, cy = y / CHUNK_SIZE, cz = z / CHUNK_SIZE;
    int lx = x % CHUNK_SIZE, ly = y % CHUNK_SIZE, lz = z % CHUNK_SIZE;
    Chunk& c = chunks[chunkIndex(cx,cy,cz)];
    c.at(lx,ly,lz) = t;
    c.dirty = true;

    // �ھ� chunk �߽����
    if(lx==0 && cx>0) chunks[chunkIndex(cx-1,cy,cz)].dirty = true;
    if(lx==CHUNK_SIZE-1 && cx<CHUNK_X-1) chunks[chunkIndex(cx+1,cy,cz)].dirty = true;
    if(lz==0 && cz>0) chunks[chunkIndex(cx,cy,cz-1)].dirty = true;
    if(lz==CHUNK_SIZE-1 && cz<CHUNK_Z-1) chunks[chunkIndex(cx,cy,cz+1)].dirty = true;
    if(ly==0 && cy>0) chunks[chunkIndex(cx,cy-1,cz)].dirty = true;
    if(ly==CHUNK_SIZE-1 && cy<CHUNK_Y-1) chunks[chunkIndex(cx,cy+1,cz)].dirty = true;
}

// ===========================
// ��ƽ�����գ�
// - ���ڡ����� AO���ھ��ڵ�����+ ���߶��չ⡱���
// ===========================
static inline float clamp01(float v){ return max(0.0f, min(1.0f, v)); }

float sunlightAt(int x,int y,int z){
    // �򻯣��߶�Խ��Խ�����������
    float h = (float)y / (WORLD_SIZE_Y - 1);
    float s = 0.35f + 0.65f * h;

    // �Ϸ��Ƿ��ڵ������ƣ�
    for(int k=y+1; k<min(WORLD_SIZE_Y, y+16); ++k){
        if(getBlock(x,k,z) != AIR){
            s *= 0.75f;
            break;
        }
    }
    return clamp01(s);
}

float aoCorner(bool side1, bool side2, bool corner){
    // MC ��� AO���������涼�� => �
    if(side1 && side2) return 0.4f;
    int occ = (int)side1 + (int)side2 + (int)corner;
    switch(occ){
        case 0: return 1.0f;
        case 1: return 0.85f;
        case 2: return 0.70f;
        default:return 0.55f;
    }
}

// ===========================
// �������ɣ������߶� + ����ֲ� + ɳ��/ˮ��
// ===========================
int heightAt(int x,int z){
    // ��һ������
    float nx = x * 0.0035f;
    float nz = z * 0.0035f;

    float h = terrainNoise.fbm(nx, nz, 5, 2.0f, 0.5f); // 0..1
    float m = terrainNoise.fbm(nx*0.5f+100, nz*0.5f-50, 3, 2.0f, 0.55f);

    // �����߶� + ���
    float base = 42.0f;
    float amp  = 28.0f;
    float mountain = pow(m, 2.2f) * 20.0f;

    int height = (int)floor(base + h * amp + mountain);
    height = max(1, min(WORLD_SIZE_Y-2, height));
    return height;
}

void generateChunk(int cx,int cy,int cz){
    Chunk& c = chunks[chunkIndex(cx,cy,cz)];
    if(c.generated) return;

    int startX = cx*CHUNK_SIZE;
    int startY = cy*CHUNK_SIZE;
    int startZ = cz*CHUNK_SIZE;

    std::mt19937 rng((cx*73856093u) ^ (cy*19349663u) ^ (cz*83492791u) ^ 20260228u);

    for(int lx=0; lx<CHUNK_SIZE; ++lx){
        for(int lz=0; lz<CHUNK_SIZE; ++lz){
            int wx = startX + lx;
            int wz = startZ + lz;
            int h = heightAt(wx,wz);

            // ɳ�������ƣ�������/γ����
            bool isSandBand = (wz < WORLD_SIZE_Z*0.28) || (wz > WORLD_SIZE_Z*0.72);

            for(int ly=0; ly<CHUNK_SIZE; ++ly){
                int wy = startY + ly;
                uint8_t t = AIR;

                if(wy <= h){
                    if(wy == h){
                        t = isSandBand ? SAND : GRASS;
                    }else if(wy >= h-3){
                        t = isSandBand ? SAND : DIRT;
                    }else{
                        // ����
                        int oreRoll = (int)(rng()%100);
                        if(oreRoll < 3 && wy < 20) t = DIAMOND;
                        else if(oreRoll < 8 && wy < 35) t = GOLD;
                        else if(oreRoll < 18 && wy < 55) t = IRON;
                        else if(oreRoll < 35) t = COAL;
                        else t = STONE;
                    }
                }else{
                    // ˮλ���̶�ˮ�棩
                    const int SEA = 38;
                    if(!isSandBand && wy <= SEA && wy > h){
                        t = WATER;
                    }
                }

                c.at(lx,ly,lz) = t;
            }

            // ����ֻ�ڵر� chunk �ﴦ��������� chunk ̫���ӣ�����򻯣�
            if(cy == (h/CHUNK_SIZE)){
                if(!isSandBand){
                    float treeN = terrainNoise.value(wx*0.07f, wz*0.07f);
                    if(treeN > 0.86f){
                        int trunkBaseY = h+1;
                        int trunkH = 4 + (int)(terrainNoise.value(wx*0.11f+20, wz*0.11f-10)*3.0f);
                        for(int ty=0; ty<trunkH; ++ty){
                            int y = trunkBaseY + ty;
                            if(inWorld(wx,y,wz)) setBlock(wx,y,wz,WOOD);
                        }
                        int top = trunkBaseY + trunkH;
                        for(int dx=-2; dx<=2; ++dx){
                            for(int dz=-2; dz<=2; ++dz){
                                for(int dy=-2; dy<=1; ++dy){
                                    int x = wx+dx, y = top+dy, z = wz+dz;
                                    if(!inWorld(x,y,z)) continue;
                                    if(dx*dx + dz*dz + dy*dy <= 6){
                                        if(getBlock(x,y,z)==AIR) setBlock(x,y,z,LEAVES);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    c.generated = true;
    c.dirty = true;
}

// ===========================
// Chunk ��Ⱦ�������ƿɼ���
// + ͸��ˮ����󻭣��򻯣�����chunk�ﻭ��������blend��
// ===========================
static inline bool isTransparent(uint8_t t){
    return t == WATER;
}

static inline bool isSolid(uint8_t t){
    return t != AIR && t != WATER;
}

// 获取方块纹理ID（0表示未加载）
inline GLuint getBlockTexture(uint8_t t) {
    if (t > 0 && t <= 11 && texturesLoaded)
        return blockTextures[t];
    return 0;
}

// 绘制一个面（纹理贴图 + 顶点 AO + 简化光照）
void emitFace(int x,int y,int z, int face, uint8_t t){
    // face: 0=+Z,1=-Z,2=+Y,3=-Y,4=+X,5=-X
    BlockType bt = (BlockType)t;
    GLuint tex = getBlockTexture(t);

    // 如果纹理可用则绑定，否则用纯色渲染
    if (tex != 0) {
        glBindTexture(GL_TEXTURE_2D, tex);
    }

    bool water = (bt == WATER);
    if(water){
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    float X=(float)x, Y=(float)y, Z=(float)z;

    // 检查周围方块遮挡
    auto occ = [&](int ox,int oy,int oz)->bool{
        return getBlock(x+ox,y+oy,z+oz) != AIR && getBlock(x+ox,y+oy,z+oz) != WATER;
    };

    float sun = sunlightAt(x,y,z);

    // 预计算方块基色（纹理不可用时的回退纯色）
    float br=1, bg=1, bb=1;
    if (tex == 0) {
        switch(bt){
            case GRASS:   br=0.2f; bg=0.8f; bb=0.2f; break;
            case DIRT:    br=0.5f; bg=0.3f; bb=0.0f; break;
            case STONE:   br=0.6f; bg=0.6f; bb=0.6f; break;
            case WOOD:    br=0.6f; bg=0.3f; bb=0.0f; break;
            case LEAVES:  br=0.0f; bg=0.6f; bb=0.0f; break;
            case SAND:    br=0.9f; bg=0.9f; bb=0.6f; break;
            case WATER:   br=0.0f; bg=0.4f; bb=0.8f; break;
            case COAL:    br=0.1f; bg=0.1f; bb=0.1f; break;
            case IRON:    br=0.7f; bg=0.7f; bb=0.7f; break;
            case GOLD:    br=1.0f; bg=0.8f; bb=0.0f; break;
            case DIAMOND: br=0.0f; bg=0.8f; bb=0.8f; break;
            default: break;
        }
    }

    // 顶点光照
    auto applyVertexLight = [&](float ao){
        float l = clamp01(0.25f + 0.75f * sun) * ao;
        float a = water ? 0.5f : 1.0f;
        if (tex != 0) {
            // 有纹理：白色灯光调制纹理颜色
            glColor4f(l, l, l, a);
        } else {
            // 无纹理：用方块基色乘以光照
            glColor4f(br * l, bg * l, bb * l, a);
        }
    };

    glBegin(GL_QUADS);

    if(face==0){ // +Z
        glNormal3f(0,0,1);
        float ao0 = aoCorner(occ(-1,0,1), occ(0,-1,1), occ(-1,-1,1));
        float ao1 = aoCorner(occ( 1,0,1), occ(0,-1,1), occ( 1,-1,1));
        float ao2 = aoCorner(occ( 1,0,1), occ(0, 1,1), occ( 1, 1,1));
        float ao3 = aoCorner(occ(-1,0,1), occ(0, 1,1), occ(-1, 1,1));
        if(tex) glTexCoord2f(0,0); applyVertexLight(ao0); glVertex3f(X,   Y,   Z+1);
        if(tex) glTexCoord2f(1,0); applyVertexLight(ao1); glVertex3f(X+1, Y,   Z+1);
        if(tex) glTexCoord2f(1,1); applyVertexLight(ao2); glVertex3f(X+1, Y+1, Z+1);
        if(tex) glTexCoord2f(0,1); applyVertexLight(ao3); glVertex3f(X,   Y+1, Z+1);
    }else if(face==1){ // -Z
        glNormal3f(0,0,-1);
        float ao0 = aoCorner(occ( 1,0,-1), occ(0,-1,-1), occ( 1,-1,-1));
        float ao1 = aoCorner(occ(-1,0,-1), occ(0,-1,-1), occ(-1,-1,-1));
        float ao2 = aoCorner(occ(-1,0,-1), occ(0, 1,-1), occ(-1, 1,-1));
        float ao3 = aoCorner(occ( 1,0,-1), occ(0, 1,-1), occ( 1, 1,-1));
        if(tex) glTexCoord2f(0,0); applyVertexLight(ao0); glVertex3f(X+1, Y,   Z);
        if(tex) glTexCoord2f(1,0); applyVertexLight(ao1); glVertex3f(X,   Y,   Z);
        if(tex) glTexCoord2f(1,1); applyVertexLight(ao2); glVertex3f(X,   Y+1, Z);
        if(tex) glTexCoord2f(0,1); applyVertexLight(ao3); glVertex3f(X+1, Y+1, Z);
    }else if(face==2){ // +Y
        glNormal3f(0,1,0);
        float ao0 = aoCorner(occ(-1,1,0), occ(0,1,-1), occ(-1,1,-1));
        float ao1 = aoCorner(occ( 1,1,0), occ(0,1,-1), occ( 1,1,-1));
        float ao2 = aoCorner(occ( 1,1,0), occ(0,1, 1), occ( 1,1, 1));
        float ao3 = aoCorner(occ(-1,1,0), occ(0,1, 1), occ(-1,1, 1));
        if(tex) glTexCoord2f(0,0); applyVertexLight(ao0); glVertex3f(X,   Y+1, Z+1);
        if(tex) glTexCoord2f(1,0); applyVertexLight(ao1); glVertex3f(X+1, Y+1, Z+1);
        if(tex) glTexCoord2f(1,1); applyVertexLight(ao2); glVertex3f(X+1, Y+1, Z);
        if(tex) glTexCoord2f(0,1); applyVertexLight(ao3); glVertex3f(X,   Y+1, Z);
    }else if(face==3){ // -Y
        glNormal3f(0,-1,0);
        float ao0 = aoCorner(occ(-1,-1,0), occ(0,-1, 1), occ(-1,-1, 1));
        float ao1 = aoCorner(occ( 1,-1,0), occ(0,-1, 1), occ( 1,-1, 1));
        float ao2 = aoCorner(occ( 1,-1,0), occ(0,-1,-1), occ( 1,-1,-1));
        float ao3 = aoCorner(occ(-1,-1,0), occ(0,-1,-1), occ(-1,-1,-1));
        if(tex) glTexCoord2f(0,0); applyVertexLight(ao0); glVertex3f(X,   Y, Z);
        if(tex) glTexCoord2f(1,0); applyVertexLight(ao1); glVertex3f(X+1, Y, Z);
        if(tex) glTexCoord2f(1,1); applyVertexLight(ao2); glVertex3f(X+1, Y, Z+1);
        if(tex) glTexCoord2f(0,1); applyVertexLight(ao3); glVertex3f(X,   Y, Z+1);
    }else if(face==4){ // +X
        glNormal3f(1,0,0);
        float ao0 = aoCorner(occ(1,0, 1), occ(1,-1,0), occ(1,-1, 1));
        float ao1 = aoCorner(occ(1,0,-1), occ(1,-1,0), occ(1,-1,-1));
        float ao2 = aoCorner(occ(1,0,-1), occ(1, 1,0), occ(1, 1,-1));
        float ao3 = aoCorner(occ(1,0, 1), occ(1, 1,0), occ(1, 1, 1));
        if(tex) glTexCoord2f(0,0); applyVertexLight(ao0); glVertex3f(X+1, Y,   Z+1);
        if(tex) glTexCoord2f(1,0); applyVertexLight(ao1); glVertex3f(X+1, Y,   Z);
        if(tex) glTexCoord2f(1,1); applyVertexLight(ao2); glVertex3f(X+1, Y+1, Z);
        if(tex) glTexCoord2f(0,1); applyVertexLight(ao3); glVertex3f(X+1, Y+1, Z+1);
    }else if(face==5){ // -X
        glNormal3f(-1,0,0);
        float ao0 = aoCorner(occ(-1,0,-1), occ(-1,-1,0), occ(-1,-1,-1));
        float ao1 = aoCorner(occ(-1,0, 1), occ(-1,-1,0), occ(-1,-1, 1));
        float ao2 = aoCorner(occ(-1,0, 1), occ(-1, 1,0), occ(-1, 1, 1));
        float ao3 = aoCorner(occ(-1,0,-1), occ(-1, 1,0), occ(-1, 1,-1));
        if(tex) glTexCoord2f(0,0); applyVertexLight(ao0); glVertex3f(X, Y,   Z);
        if(tex) glTexCoord2f(1,0); applyVertexLight(ao1); glVertex3f(X, Y,   Z+1);
        if(tex) glTexCoord2f(1,1); applyVertexLight(ao2); glVertex3f(X, Y+1, Z+1);
        if(tex) glTexCoord2f(0,1); applyVertexLight(ao3); glVertex3f(X, Y+1, Z);
    }

    glEnd();

    if(water){
        glDisable(GL_BLEND);
    }
}

void rebuildChunkMesh(int cx,int cy,int cz){
    generateChunk(cx,cy,cz);
    Chunk& c = chunks[chunkIndex(cx,cy,cz)];

    if(c.displayList == 0) c.displayList = glGenLists(1);
    glNewList(c.displayList, GL_COMPILE);

    int baseX = cx*CHUNK_SIZE;
    int baseY = cy*CHUNK_SIZE;
    int baseZ = cz*CHUNK_SIZE;

    // �Ȼ�ʵ�壬�ٻ�ˮ�������飩
    for(int pass=0; pass<2; ++pass){
        for(int lx=0; lx<CHUNK_SIZE; ++lx){
            for(int ly=0; ly<CHUNK_SIZE; ++ly){
                for(int lz=0; lz<CHUNK_SIZE; ++lz){
                    uint8_t t = c.atc(lx,ly,lz);
                    if(t==AIR) continue;

                    bool transp = isTransparent(t);
                    if(pass==0 && transp) continue;
                    if(pass==1 && !transp) continue;

                    int x = baseX+lx, y=baseY+ly, z=baseZ+lz;

                    // ֻ���ƿɼ��棺�ھ�Ϊ�� or �ھ�͸��/��ͬ��͸��
                    auto faceVisible = [&](int nx,int ny,int nz)->bool{
                        uint8_t nt = getBlock(nx,ny,nz);
                        if(nt==AIR) return true;
                        // ˮ��ʵ�壺ʵ��������ˮҲҪ����ˮ������ʵ��ҲҪ��
                        if(t==WATER && nt!=WATER) return true;
                        if(t!=WATER && nt==WATER) return true;
                        return false;
                    };

                    if(faceVisible(x, y, z+1)) emitFace(x,y,z,0,t);
                    if(faceVisible(x, y, z-1)) emitFace(x,y,z,1,t);
                    if(faceVisible(x, y+1, z)) emitFace(x,y,z,2,t);
                    if(faceVisible(x, y-1, z)) emitFace(x,y,z,3,t);
                    if(faceVisible(x+1, y, z)) emitFace(x,y,z,4,t);
                    if(faceVisible(x-1, y, z)) emitFace(x,y,z,5,t);
                }
            }
        }
    }

    glEndList();
    c.dirty = false;
}

// ===========================
// UI���ƣ�������MC�����λ��
// ===========================
void drawRect(float x,float y,float w,float h, float r,float g,float b,float a){
    glColor4f(r,g,b,a);
    glBegin(GL_QUADS);
    glVertex2f(x,y);
    glVertex2f(x+w,y);
    glVertex2f(x+w,y+h);
    glVertex2f(x,y+h);
    glEnd();
}

void drawBevelSlot(float x,float y,float s, bool highlight=false){
    // ����
    drawRect(x,y,s,s, 0.15f,0.15f,0.15f,0.85f);
    // �ڿ�
    glColor4f(0.05f,0.05f,0.05f,0.9f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x+1,y+1);
    glVertex2f(x+s-1,y+1);
    glVertex2f(x+s-1,y+s-1);
    glVertex2f(x+1,y+s-1);
    glEnd();
    // ���߹�/��Ӱ
    if(highlight){
        glColor3f(1.0f,1.0f,0.2f);
        glLineWidth(2);
    }else{
        glColor3f(0.55f,0.55f,0.55f);
        glLineWidth(1);
    }
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,y);
    glVertex2f(x+s,y);
    glVertex2f(x+s,y+s);
    glVertex2f(x,y+s);
    glEnd();
    glLineWidth(1);
}

void drawItemIcon(const Item& item, float x, float y, float size) {
    if (item.isEmpty()) return;

    // ��Ʒ��
    drawRect(x+4, y+4, size-8, size-8, 0.2f,0.2f,0.2f,0.2f);

    // ��Ʒɫ��
    setItemColor(item.type);
    glBegin(GL_QUADS);
    glVertex2f(x+8, y+8);
    glVertex2f(x+size-8, y+8);
    glVertex2f(x+size-8, y+size-8);
    glVertex2f(x+8, y+size-8);
    glEnd();

    // ���߼򵥷��ţ�������ԭ���
    if (item.type >= WOODEN_PICKAXE && item.type <= DIAMOND_SHOVEL) {
        glColor3f(0.0f,0.0f,0.0f);
        float toolSize = size / 3;

        // ��
        if (item.type==WOODEN_PICKAXE||item.type==STONE_PICKAXE||item.type==IRON_PICKAXE||item.type==GOLDEN_PICKAXE||item.type==DIAMOND_PICKAXE) {
            glBegin(GL_QUADS);
            glVertex2f(x+size/2-toolSize/2, y+size/2-toolSize/2);
            glVertex2f(x+size/2+toolSize/2, y+size/2-toolSize/2);
            glVertex2f(x+size/2+toolSize/2, y+size/2+toolSize/2);
            glVertex2f(x+size/2-toolSize/2, y+size/2+toolSize/2);
            glEnd();

            glBegin(GL_QUADS);
            glVertex2f(x+size/2-toolSize/6, y+size/2-toolSize/2-toolSize/3);
            glVertex2f(x+size/2+toolSize/6, y+size/2-toolSize/2-toolSize/3);
            glVertex2f(x+size/2+toolSize/6, y+size/2+toolSize/2+toolSize/3);
            glVertex2f(x+size/2-toolSize/6, y+size/2+toolSize/2+toolSize/3);
            glEnd();
        }
        // ��
        else if (item.type==WOODEN_AXE||item.type==STONE_AXE||item.type==IRON_AXE||item.type==GOLDEN_AXE||item.type==DIAMOND_AXE) {
            glBegin(GL_QUADS);
            glVertex2f(x+size/2-toolSize/2, y+size/2-toolSize/2);
            glVertex2f(x+size/2+toolSize/2, y+size/2-toolSize/2);
            glVertex2f(x+size/2+toolSize/2, y+size/2+toolSize/2);
            glVertex2f(x+size/2-toolSize/2, y+size/2+toolSize/2);
            glEnd();

            glBegin(GL_QUADS);
            glVertex2f(x+size/2+toolSize/2, y+size/2-toolSize/2);
            glVertex2f(x+size/2+toolSize/2+toolSize/3, y+size/2);
            glVertex2f(x+size/2+toolSize/2, y+size/2+toolSize/2);
            glVertex2f(x+size/2+toolSize/6, y+size/2+toolSize/3);
            glEnd();
        }
        // ��
        else {
            glBegin(GL_TRIANGLES);
            glVertex2f(x+size/2, y+size/2-toolSize/2);
            glVertex2f(x+size/2+toolSize/2, y+size/2+toolSize/2);
            glVertex2f(x+size/2-toolSize/2, y+size/2+toolSize/2);
            glEnd();

            glBegin(GL_QUADS);
            glVertex2f(x+size/2-toolSize/6, y+size/2-toolSize/2-toolSize/3);
            glVertex2f(x+size/2+toolSize/6, y+size/2-toolSize/2-toolSize/3);
            glVertex2f(x+size/2+toolSize/6, y+size/2-toolSize/2);
            glVertex2f(x+size/2-toolSize/6, y+size/2-toolSize/2);
            glEnd();
        }
    }

    // ����
    if(item.count > 1 && item.type < WOODEN_PICKAXE){
        glColor3f(1,1,1);
        glRasterPos2f(x+size-18, y+6);
        string s = to_string(item.count);
        for(char c: s) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
    }
}

void begin2D(){
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_W, 0, WINDOW_H);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
}

void end2D(){
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawRenderDistance(){
    if(playerInventory.showInventory || playerInventory.showCrafting) return;

    begin2D();

    float x = 20.0f;
    float y = WINDOW_H - 40.0f;

    // 半透明背景
    drawRect(x-6, y-4, 220, 30, 0.0f, 0.0f, 0.0f, 0.4f);

    // 白色文字
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);

    char buf[64];
    int chunksPerSide = VIEW_CHUNK_RADIUS * 2 + 1;
    snprintf(buf, sizeof(buf), "Render: %d (%dx%d chunks) [+/-]", VIEW_CHUNK_RADIUS, chunksPerSide, chunksPerSide);
    for (char* c = buf; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    end2D();
}

void drawHotbar(){
    if(playerInventory.showInventory || playerInventory.showCrafting) return;

    begin2D();

    float slot = 52.0f;
    float gap  = 6.0f;
    float totalW = HOTBAR_SIZE*slot + (HOTBAR_SIZE-1)*gap;
    float x0 = (WINDOW_W - totalW)/2.0f;
    float y0 = 30.0f;

    // ������������MC��
    drawRect(x0-10, y0-10, totalW+20, slot+20, 0.05f,0.05f,0.05f,0.55f);

    for(int i=0;i<HOTBAR_SIZE;i++){
        float x = x0 + i*(slot+gap);
        bool hi = (i==playerInventory.selectedHotbarSlot);
        drawBevelSlot(x,y0,slot,hi);

        // ���
        glColor3f(1,1,1);
        glRasterPos2f(x+4, y0+slot-14);
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '1'+i);

        drawItemIcon(playerInventory.hotbar[i], x, y0, slot);
    }

    end2D();
}

void drawInventoryUI(){
    if(!playerInventory.showInventory && !playerInventory.showCrafting) return;

    begin2D();

    // ��������
    drawRect(0,0,WINDOW_W,WINDOW_H, 0,0,0,0.55f);

    // ���
    float panelW = 800;
    float panelH = 560;
    float px = (WINDOW_W - panelW)/2.0f;
    float py = (WINDOW_H - panelH)/2.0f;
    drawRect(px,py,panelW,panelH, 0.25f,0.25f,0.25f,0.92f);
    glColor3f(0.6f,0.6f,0.6f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(px,py);
    glVertex2f(px+panelW,py);
    glVertex2f(px+panelW,py+panelH);
    glVertex2f(px,py+panelH);
    glEnd();

    // ����
    glColor3f(1,1,1);
    glRasterPos2f(px+20, py+panelH-40);
    const char* title = playerInventory.showCrafting ? "Crafting" : "Inventory";
    for(const char* c=title; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    float slot = 48.0f;
    float gap = 6.0f;

    // ��������̶������ײ���
    float hbW = HOTBAR_SIZE*slot + (HOTBAR_SIZE-1)*gap;
    float hbX = px + (panelW-hbW)/2.0f;
    float hbY = py + 40;

    for(int i=0;i<HOTBAR_SIZE;i++){
        float x = hbX + i*(slot+gap);
        bool hi = (i==playerInventory.selectedHotbarSlot);
        drawBevelSlot(x,hbY,slot,hi);
        drawItemIcon(playerInventory.hotbar[i], x, hbY, slot);
    }

    // ������ 9x3����Ӧ items[0..26]��+ ����һ�� 9 ��items[27..35]��=> 4�й�36
    if(playerInventory.showInventory){
        float invX = hbX;
        float invY = hbY + slot + 26;

        int idx=0;
        for(int r=0;r<4;r++){
            for(int c=0;c<9;c++){
                float x = invX + c*(slot+gap);
                float y = invY + (3-r)*(slot+gap); // ��������
                drawBevelSlot(x,y,slot,false);
                drawItemIcon(playerInventory.items[idx], x,y,slot);
                idx++;
            }
        }
    }

    // �ϳ� 3x3 + ���
    if(playerInventory.showCrafting){
        float gridX = px + 120;
        float gridY = py + 220;

        for(int r=0;r<3;r++){
            for(int c=0;c<3;c++){
                int idx = r*3+c;
                float x = gridX + c*(slot+gap);
                float y = gridY + (2-r)*(slot+gap);
                drawBevelSlot(x,y,slot,false);
                drawItemIcon(playerInventory.craftingGrid[idx], x,y,slot);
            }
        }

        // ��ͷ
        glColor3f(1,1,1);
        float ax = gridX + 3*(slot+gap) + 20;
        float ay = gridY + slot + 20;
        glBegin(GL_TRIANGLES);
        glVertex2f(ax, ay);
        glVertex2f(ax+30, ay+15);
        glVertex2f(ax+30, ay-15);
        glEnd();

        // �����
        float rx = ax + 60;
        float ry = gridY + slot;
        drawBevelSlot(rx,ry,slot,false);
        drawItemIcon(playerInventory.craftingResult, rx,ry,slot);
    }

    end2D();
}

void drawCrosshair(){
    if(playerInventory.showInventory || playerInventory.showCrafting) return;
    begin2D();
    float cx = WINDOW_W/2.0f;
    float cy = WINDOW_H/2.0f;

    glColor3f(1,1,1);
    glLineWidth(2);
    glBegin(GL_LINES);
    glVertex2f(cx-10, cy); glVertex2f(cx-2, cy);
    glVertex2f(cx+2, cy);  glVertex2f(cx+10, cy);
    glVertex2f(cx, cy-10); glVertex2f(cx, cy-2);
    glVertex2f(cx, cy+2);  glVertex2f(cx, cy+10);
    glEnd();
    glLineWidth(1);

    end2D();
}

// ===========================
// �ϳ��䷽����������䷽����ԭ��ľ�䡱�͡�ľ�����䷽һ�������ﲻɾ��
// ===========================
void initCraftingRecipes(){
    ItemType woodenPickaxeIngredients[9] = {
        WOOD_BLOCK, WOOD_BLOCK, AIR_ITEM,
        WOOD_BLOCK, AIR_ITEM,  AIR_ITEM,
        AIR_ITEM,   WOOD_BLOCK, AIR_ITEM
    };
    craftingRecipes.push_back(Recipe(WOODEN_PICKAXE, 1, woodenPickaxeIngredients));

    ItemType woodenAxeIngredients[9] = {
        WOOD_BLOCK, WOOD_BLOCK, AIR_ITEM,
        WOOD_BLOCK, WOOD_BLOCK,  AIR_ITEM,
        AIR_ITEM,   WOOD_BLOCK, AIR_ITEM
    };
    craftingRecipes.push_back(Recipe(WOODEN_AXE, 1, woodenAxeIngredients));

    ItemType woodenShovelIngredients[9] = {
        WOOD_BLOCK, AIR_ITEM, AIR_ITEM,
        WOOD_BLOCK, AIR_ITEM, AIR_ITEM,
        AIR_ITEM,   WOOD_BLOCK, AIR_ITEM
    };
    craftingRecipes.push_back(Recipe(WOODEN_SHOVEL, 1, woodenShovelIngredients));
}

// ===========================
// �����㣺�ҵر������⿨����
// ===========================
bool aabbCollides(float px,float py,float pz){
    // ���AABB��x/z �뾶 PLAYER_RADIUS���� PLAYER_HEIGHT
    float minX = px - PLAYER_RADIUS;
    float maxX = px + PLAYER_RADIUS;
    float minY = py;
    float maxY = py + PLAYER_HEIGHT;
    float minZ = pz - PLAYER_RADIUS;
    float maxZ = pz + PLAYER_RADIUS;

    int x0 = (int)floor(minX), x1=(int)floor(maxX);
    int y0 = (int)floor(minY), y1=(int)floor(maxY);
    int z0 = (int)floor(minZ), z1=(int)floor(maxZ);

    for(int x=x0;x<=x1;x++){
        for(int y=y0;y<=y1;y++){
            for(int z=z0;z<=z1;z++){
                uint8_t t = getBlock(x,y,z);
                if(t!=AIR && t!=WATER){
                    return true;
                }
            }
        }
    }
    return false;
}

void findSpawn(){
    int sx = WORLD_SIZE_X/2;
    int sz = WORLD_SIZE_Z/2;
    int h = heightAt(sx,sz);
    playerX = sx + 0.5f;
    playerZ = sz + 0.5f;
    playerY = h + 2.0f;

    // ȷ������ģ�������ҿ�
    for(int i=0;i<40;i++){
        if(!aabbCollides(playerX,playerY,playerZ)) break;
        playerY += 1.0f;
    }
}

// ===========================
// ����ʰȡ���ƻ�/���ã�
// ===========================
struct RayHit{
    bool hit=false;
    int bx=0,by=0,bz=0;
    int px=0,py=0,pz=0; // ����λ�ã���һ��������
};

RayHit raycast(float maxDist){
    RayHit rh;
    float ox=playerX, oy=playerY+1.62f, oz=playerZ; // ���߸߶�
    float dx = sin(playerRotY) * cos(playerRotX);
    float dy = sin(playerRotX);
    float dz = cos(playerRotY) * cos(playerRotX);

    float len = sqrt(dx*dx+dy*dy+dz*dz);
    dx/=len; dy/=len; dz/=len;

    float t=0;
    int lastX=(int)floor(ox), lastY=(int)floor(oy), lastZ=(int)floor(oz);

    for(; t<maxDist; t+=0.05f){
        float x = ox + dx*t;
        float y = oy + dy*t;
        float z = oz + dz*t;

        int bx=(int)floor(x);
        int by=(int)floor(y);
        int bz=(int)floor(z);

        if(!inWorld(bx,by,bz)) continue;

        uint8_t bt = getBlock(bx,by,bz);
        if(bt!=AIR && bt!=WATER){
            rh.hit=true;
            rh.bx=bx; rh.by=by; rh.bz=bz;
            rh.px=lastX; rh.py=lastY; rh.pz=lastZ;
            return rh;
        }
        lastX=bx; lastY=by; lastZ=bz;
    }
    return rh;
}

ItemType blockToItem(uint8_t bt){
    switch((BlockType)bt){
        case GRASS: return GRASS_BLOCK;
        case DIRT: return DIRT_BLOCK;
        case STONE: return STONE_BLOCK;
        case WOOD: return WOOD_BLOCK;
        case LEAVES: return LEAVES_BLOCK;
        case SAND: return SAND_BLOCK;
        case WATER: return WATER_BLOCK;
        case COAL: return COAL_BLOCK;
        case IRON: return IRON_BLOCK;
        case GOLD: return GOLD_BLOCK;
        case DIAMOND: return DIAMOND_BLOCK;
        default: return AIR_ITEM;
    }
}

uint8_t itemToBlock(ItemType it){
    switch(it){
        case GRASS_BLOCK: return GRASS;
        case DIRT_BLOCK: return DIRT;
        case STONE_BLOCK: return STONE;
        case WOOD_BLOCK: return WOOD;
        case LEAVES_BLOCK: return LEAVES;
        case SAND_BLOCK: return SAND;
        case WATER_BLOCK: return WATER;
        case COAL_BLOCK: return COAL;
        case IRON_BLOCK: return IRON;
        case GOLD_BLOCK: return GOLD;
        case DIAMOND_BLOCK: return DIAMOND;
        default: return AIR;
    }
}

// ===========================
// �˶� + ��ײ�������ƶ���
// ===========================
void tryMoveAxis(float& px,float& py,float& pz, float dx,float dy,float dz){
    // X
    if(dx!=0){
        float nx = px + dx;
        if(!aabbCollides(nx,py,pz)) px = nx;
        else{
            // ��ǽ�����԰�λ����С���򵥣�
            float step = dx>0 ? 0.01f : -0.01f;
            while(fabs(dx) > 0.0001f){
                float tx = px + dx;
                if(!aabbCollides(tx,py,pz)){ px = tx; break; }
                dx -= step;
            }
        }
    }
    // Z
    if(dz!=0){
        float nz = pz + dz;
        if(!aabbCollides(px,py,nz)) pz = nz;
        else{
            float step = dz>0 ? 0.01f : -0.01f;
            while(fabs(dz) > 0.0001f){
                float tz = pz + dz;
                if(!aabbCollides(px,py,tz)){ pz = tz; break; }
                dz -= step;
            }
        }
    }
    // Y
    if(dy!=0){
        float ny = py + dy;
        if(!aabbCollides(px,ny,pz)){
            py = ny;
            onGround = false;
        }else{
            // ��������/�컨��
            float step = dy>0 ? 0.01f : -0.01f;
            while(fabs(dy) > 0.0001f){
                float ty = py + dy;
                if(!aabbCollides(px,ty,pz)){ py = ty; break; }
                dy -= step;
            }
            if(step < 0) onGround = true; // ����ײ�� => ���
            velY = 0;
        }
    }
}

void updatePhysics(float dt){
    // ���
    if(playerY < -20.0f){
        exit(0);
    }

    // ����
    velY -= GRAVITY * dt;

    // ���뷽��WASD��
    float forward = 0, right = 0;
    if(keyDown['w'] || keyDown['W']) forward += 1;
    if(keyDown['s'] || keyDown['S']) forward -= 1;
    if(keyDown['d'] || keyDown['D']) right   -= 1;
    if(keyDown['a'] || keyDown['A']) right   += 1;

    float yaw = playerRotY;
    float fx = sin(yaw), fz = cos(yaw);
    float rx = cos(yaw), rz = -sin(yaw);

    float mx = fx*forward + rx*right;
    float mz = fz*forward + rz*right;

    float mag = sqrt(mx*mx + mz*mz);
    if(mag > 0.001f){
        mx/=mag; mz/=mag;
    }

    float targetVX = mx * WALK_SPEED;
    float targetVZ = mz * WALK_SPEED;

    // �򵥼��ٶ�/Ħ��
    float accel = onGround ? 40.0f : 10.0f;
    velX += (targetVX - velX) * min(1.0f, accel*dt);
    velZ += (targetVZ - velZ) * min(1.0f, accel*dt);

    // ��Ծ
    if((keyDown[' '] || specialDown[GLUT_KEY_UP]) && onGround){
        // v = sqrt(2gh)
        velY = sqrt(2.0f * GRAVITY * JUMP_HEIGHT);
        onGround = false;
    }

    // �����ƶ�
    float dx = velX * dt;
    float dy = velY * dt;
    float dz = velZ * dt;

    tryMoveAxis(playerX,playerY,playerZ, dx,dy,dz);

    // ����߽� clamp������ɳ���
    playerX = max(0.5f, min((float)WORLD_SIZE_X-0.5f, playerX));
    playerZ = max(0.5f, min((float)WORLD_SIZE_Z-0.5f, playerZ));
}

// ===========================
// ��Ⱦ
// ===========================
void setupCamera(){
    glLoadIdentity();
    float eyeX = playerX;
    float eyeY = playerY + 1.62f;
    float eyeZ = playerZ;

    float dirX = sin(playerRotY) * cos(playerRotX);
    float dirY = sin(playerRotX);
    float dirZ = cos(playerRotY) * cos(playerRotX);

    gluLookAt(eyeX,eyeY,eyeZ,
              eyeX+dirX, eyeY+dirY, eyeZ+dirZ,
              0,1,0);
}

void renderWorld(){
    // ֻ��Ⱦ�Ӿ��� chunks
    int pcx = (int)floor(playerX) / CHUNK_SIZE;
    int pcy = (int)floor(playerY) / CHUNK_SIZE;
    int pcz = (int)floor(playerZ) / CHUNK_SIZE;

    int minCX = max(0, pcx - VIEW_CHUNK_RADIUS);
    int maxCX = min(CHUNK_X-1, pcx + VIEW_CHUNK_RADIUS);
    int minCZ = max(0, pcz - VIEW_CHUNK_RADIUS);
    int maxCZ = min(CHUNK_Z-1, pcz + VIEW_CHUNK_RADIUS);
    int minCY = 0;
    int maxCY = CHUNK_Y-1;

    for(int cy=minCY; cy<=maxCY; ++cy){
        for(int cz=minCZ; cz<=maxCZ; ++cz){
            for(int cx=minCX; cx<=maxCX; ++cx){
                Chunk& c = chunks[chunkIndex(cx,cy,cz)];
                if(!c.generated) generateChunk(cx,cy,cz);
                if(c.dirty) rebuildChunkMesh(cx,cy,cz);
                if(c.displayList) glCallList(c.displayList);
            }
        }
    }
}

void renderScene(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    setupCamera();

    renderWorld();

    // UI
    drawHotbar();
    drawInventoryUI();
    drawCrosshair();
    drawRenderDistance();

    glutSwapBuffers();
}

// ===========================
// OpenGL ��ʼ��
// ===========================
void initGL(){
    glClearColor(0.5f,0.7f,1.0f,1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // 简化光照：禁用固定管线光照，用顶点颜色+AO/日光
    glDisable(GL_LIGHTING);

    // 启用纹理映射
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // 加载所有方块 BMP 贴图
    loadAllTextures();
}

// ===========================
// reshape
// ===========================
void reshape(int w,int h){
    if(h==0) h=1;
    float ratio = (float)w/(float)h;

    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70.0, ratio, 0.05, 500.0);
    glMatrixMode(GL_MODELVIEW);
}

// ===========================
// ���̣�֧�ֶ��
// ===========================
void keyDownFunc(unsigned char key,int,int){
    keyDown[(unsigned char)key] = true;

    switch(key){
        case 'e':
        case 'E':
            playerInventory.showInventory = !playerInventory.showInventory;
            playerInventory.showCrafting = false;
            if(playerInventory.showInventory){
                lockMouse = false;
                glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
            }else{
                lockMouse = true;
                glutSetCursor(GLUT_CURSOR_NONE);
                firstMouse = true;
                glutWarpPointer(CENTER_X, CENTER_Y);
            }
            break;
        case 'q':
        case 'Q':
            playerInventory.showCrafting = !playerInventory.showCrafting;
            playerInventory.showInventory = false;
            if(playerInventory.showCrafting){
                playerInventory.checkCraftingRecipes(craftingRecipes);
                lockMouse = false;
                glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
            }else{
                lockMouse = true;
                glutSetCursor(GLUT_CURSOR_NONE);
                firstMouse = true;
                glutWarpPointer(CENTER_X, CENTER_Y);
            }
            break;
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            playerInventory.selectedHotbarSlot = key - '1';
            break;
        case '+': case '=':
            // 增大渲染距离 (最大 16)
            if (VIEW_CHUNK_RADIUS < 16) {
                VIEW_CHUNK_RADIUS++;
                // 标记所有已生成的chunk为脏，触发重建
                for (auto& c : chunks) if (c.generated) c.dirty = true;
                printf("渲染距离: %d (%.1fx%.1f chunks)\n",
                       VIEW_CHUNK_RADIUS,
                       (float)(VIEW_CHUNK_RADIUS*2+1),
                       (float)(VIEW_CHUNK_RADIUS*2+1));
            }
            break;
        case '-': case '_':
            // 减小渲染距离 (最小 2)
            if (VIEW_CHUNK_RADIUS > 2) {
                VIEW_CHUNK_RADIUS--;
                for (auto& c : chunks) if (c.generated) c.dirty = true;
                printf("渲染距离: %d (%.1fx%.1f chunks)\n",
                       VIEW_CHUNK_RADIUS,
                       (float)(VIEW_CHUNK_RADIUS*2+1),
                       (float)(VIEW_CHUNK_RADIUS*2+1));
            }
            break;
        case 27:
            exit(0);
            break;
    }
}

void keyUpFunc(unsigned char key,int,int){
    keyDown[(unsigned char)key] = false;
}

void specialDownFunc(int key,int,int){
    specialDown[key] = true;
}
void specialUpFunc(int key,int,int){
    specialDown[key] = false;
}

// ===========================
// ��꣺�����ģ�����ƶ�һ��
// ===========================
void passiveMouse(int x,int y){
    if(!lockMouse) return;

    if(firstMouse){
        firstMouse = false;
        glutWarpPointer(CENTER_X, CENTER_Y);
        return;
    }

    int dx = x - CENTER_X;
    int dy = y - CENTER_Y;

    playerRotY -= dx * MOUSE_SENS;
    playerRotX -= dy * MOUSE_SENS; // �����ƶ� -> pitch���ӣ������ü�������ֱ����

    // pitch ����
    float limit = 1.55f;
    if(playerRotX > limit) playerRotX = limit;
    if(playerRotX < -limit) playerRotX = -limit;

    glutWarpPointer(CENTER_X, CENTER_Y);
}

// ===========================
// �������
// - UI���������/�ϳɣ�
// - ��Ϸ������ƻ����Ҽ�����
// ===========================
bool pointIn(float mx,float my,float x,float y,float w,float h){
    return mx>=x && mx<=x+w && my>=y && my<=y+h;
}

// �ӿ����/����ȡ 1 ���Ž��ϳɸ񣨲���������գ�
bool takeOneFromSelected(Item& out){
    Item sel = playerInventory.hotbar[playerInventory.selectedHotbarSlot];
    if(sel.isEmpty()) return false;
    out = Item(sel.type, 1);
    sel.count--;
    if(sel.count<=0) playerInventory.hotbar[playerInventory.selectedHotbarSlot] = Item();
    else playerInventory.hotbar[playerInventory.selectedHotbarSlot] = sel;
    return true;
}

void mouseClick(int button,int state,int x,int y){
    if(state != GLUT_DOWN) return;

    // GLUT �� y ���ϵ��£�����ת��������2D���꣨�µ��ϣ�
    float mx = (float)x;
    float my = (float)(WINDOW_H - y);

    // UI����
    if(playerInventory.showInventory || playerInventory.showCrafting){
        float panelW = 800, panelH = 560;
        float px = (WINDOW_W - panelW)/2.0f;
        float py = (WINDOW_H - panelH)/2.0f;

        float slot = 48.0f;
        float gap = 6.0f;
        float hbW = HOTBAR_SIZE*slot + (HOTBAR_SIZE-1)*gap;
        float hbX = px + (panelW-hbW)/2.0f;
        float hbY = py + 40;

        // ����������ѡ���λ
        for(int i=0;i<9;i++){
            float sx = hbX + i*(slot+gap);
            if(pointIn(mx,my,sx,hbY,slot,slot)){
                playerInventory.selectedHotbarSlot = i;
                return;
            }
        }

        // ����UI�������������� <-> ������
        if(playerInventory.showInventory){
            float invX = hbX;
            float invY = hbY + slot + 26;

            int idx=0;
            for(int r=0;r<4;r++){
                for(int c=0;c<9;c++){
                    float sx = invX + c*(slot+gap);
                    float sy = invY + (3-r)*(slot+gap);
                    if(pointIn(mx,my,sx,sy,slot,slot)){
                        Item tmp = playerInventory.hotbar[playerInventory.selectedHotbarSlot];
                        playerInventory.hotbar[playerInventory.selectedHotbarSlot] = playerInventory.items[idx];
                        playerInventory.items[idx] = tmp;
                        return;
                    }
                    idx++;
                }
            }
        }

        // �ϳ�UI������������/ȡ���
        if(playerInventory.showCrafting){
            float gridX = px + 120;
            float gridY = py + 220;

            for(int r=0;r<3;r++){
                for(int c=0;c<3;c++){
                    int idx = r*3+c;
                    float sx = gridX + c*(slot+gap);
                    float sy = gridY + (2-r)*(slot+gap);

                    if(pointIn(mx,my,sx,sy,slot,slot)){
                        // �� 1 ��
                        Item one;
                        if(takeOneFromSelected(one)){
                            // ���������ͬ��ɶѵ�
                            if(!playerInventory.craftingGrid[idx].isEmpty() &&
                               playerInventory.craftingGrid[idx].type == one.type &&
                               playerInventory.craftingGrid[idx].count < 64){
                                playerInventory.craftingGrid[idx].count++;
                            }else if(playerInventory.craftingGrid[idx].isEmpty()){
                                playerInventory.craftingGrid[idx] = one;
                            }else{
                                // ��ͬ�ࣺ�򵥽�����ȥ�����ⶪʧ��
                                playerInventory.addItem(one);
                            }
                            playerInventory.checkCraftingRecipes(craftingRecipes);
                        }
                        return;
                    }
                }
            }

            // �����
            float ax = gridX + 3*(slot+gap) + 20;
            float rx = ax + 60;
            float ry = gridY + slot;

            if(pointIn(mx,my,rx,ry,slot,slot)){
                if(playerInventory.craftItem()){
                    playerInventory.checkCraftingRecipes(craftingRecipes);
                }
                return;
            }
        }

        return;
    }

    // ���罻��
    RayHit hit = raycast(6.0f);
    if(!hit.hit) return;

    if(button == GLUT_LEFT_BUTTON){
        uint8_t bt = getBlock(hit.bx,hit.by,hit.bz);
        setBlock(hit.bx,hit.by,hit.bz, AIR);

        // �ƻ����� -> ���� +1���ҿɶѵ�
        ItemType it = blockToItem(bt);
        if(it != AIR_ITEM){
            playerInventory.addItem(Item(it, 1));
        }
    }
    else if(button == GLUT_RIGHT_BUTTON){
        Item sel = playerInventory.getSelectedItem();
        if(sel.isEmpty()) return;

        // ���÷���
        if(sel.type >= GRASS_BLOCK && sel.type <= DIAMOND_BLOCK){
            int px = hit.px, py = hit.py, pz = hit.pz;
            if(!inWorld(px,py,pz)) return;

            // ���ܷŽ��������
            uint8_t placeType = itemToBlock(sel.type);
            if(placeType==AIR) return;

            // �ղŷ�
            if(getBlock(px,py,pz)==AIR){
                // ��ʱ���ú�����Ҵ�ģ
                setBlock(px,py,pz, placeType);
                if(aabbCollides(playerX,playerY,playerZ)){
                    // �Ż�ȥ
                    setBlock(px,py,pz, AIR);
                    return;
                }

                // ����
                sel.count--;
                if(sel.count<=0) playerInventory.hotbar[playerInventory.selectedHotbarSlot] = Item();
                else playerInventory.hotbar[playerInventory.selectedHotbarSlot] = sel;
            }
        }
        // �����߼��ɼ�����չ����ɾ��Ĺ��ܣ�
    }
}

// ===========================
// ÿ֡���£�����+����+chunk������ڣ�
// ===========================
void fixedUpdate(){
    static double last = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    static double acc = 0.0;

    double now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    double dt = now - last;
    last = now;
    dt = min(dt, 0.05); // ������

    // UI����ʱ������������������Ⱦ��
    if(playerInventory.showInventory || playerInventory.showCrafting){
        glutPostRedisplay();
        return;
    }

    acc += dt;
    while(acc >= FIXED_DT){
        updatePhysics((float)FIXED_DT);

        // ÿ֡��Ҫ������ڣ�����/��Ⱦ/������ȣ�
        // �����⣺�����⿨�룬���ϵ���
        if(aabbCollides(playerX,playerY,playerZ)){
            for(int i=0;i<10;i++){
                playerY += 0.1f;
                if(!aabbCollides(playerX,playerY,playerZ)) break;
            }
        }

        acc -= FIXED_DT;
    }

    glutPostRedisplay();
}

// ===========================
// �����ʼ����ֻԤ������Ҹ��� chunk�����ఴ�����ɣ�
// ===========================
void initWorld(){
    // ֻ��ǰ����һȦ����������
    int pcx = (int)floor(playerX) / CHUNK_SIZE;
    int pcy = (int)floor(playerY) / CHUNK_SIZE;
    int pcz = (int)floor(playerZ) / CHUNK_SIZE;

    for(int cy=0; cy<CHUNK_Y; ++cy){
        for(int dz=-VIEW_CHUNK_RADIUS; dz<=VIEW_CHUNK_RADIUS; ++dz){
            for(int dx=-VIEW_CHUNK_RADIUS; dx<=VIEW_CHUNK_RADIUS; ++dx){
                int cx = pcx + dx;
                int cz = pcz + dz;
                if(cx<0||cx>=CHUNK_X||cz<0||cz>=CHUNK_Z) continue;
                generateChunk(cx,cy,cz);
            }
        }
    }
}

// ===========================
// main
// ===========================
int main(int argc,char** argv){
#ifdef _WIN32
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
    memset(keyDown, 0, sizeof(keyDown));
    memset(specialDown, 0, sizeof(specialDown));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_W, WINDOW_H);
    glutCreateWindow("Minecraft 3D");

    initGL();
    initCraftingRecipes();

    // �����㣨���⴩ģ��
    findSpawn();

    // ��ʼ�����磨chunk�������ɣ�
    initWorld();

    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshape);

    glutKeyboardFunc(keyDownFunc);
    glutKeyboardUpFunc(keyUpFunc);
    glutSpecialFunc(specialDownFunc);
    glutSpecialUpFunc(specialUpFunc);

    glutPassiveMotionFunc(passiveMouse);
    glutMouseFunc(mouseClick);

    glutIdleFunc(fixedUpdate);

    // ������ز�����
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(CENTER_X, CENTER_Y);

    glutMainLoop();
    return 0;
}
