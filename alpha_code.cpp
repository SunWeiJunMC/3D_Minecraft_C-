#include <GL/glut.h>
#include <bits/stdc++.h>
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500
#endif
#include <windows.h>
using namespace std;
// 定义世界尺寸
const int WORLD_SIZE_X = 128;
const int WORLD_SIZE_Y = 64;
const int WORLD_SIZE_Z = 128;
const int HOTBAR_SIZE = 9;
const int INVENTORY_SIZE = 36;
const int CRAFTING_GRID_SIZE = 3;
// 物品类型（包括方块和工具）
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
// 方块类型
enum BlockType {
    AIR,
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
// 方块结构
struct Block {
    BlockType type;
    bool visible;

    Block() : type(AIR), visible(false) {}
    Block(BlockType t) : type(t), visible(true) {}
};

// 物品结构
struct Item {
    ItemType type;
    int count;

    Item() : type(AIR_ITEM), count(0) {}
    Item(ItemType t, int c = 1) : type(t), count(c) {}

    bool isEmpty() const {
        return type == AIR_ITEM || count <= 0;
    }
};

// 合成配方
struct Recipe {
    ItemType result;
    int resultCount;
    ItemType ingredients[CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE];

    Recipe(ItemType res, int resCount, const ItemType* ingr) 
        : result(res), resultCount(resCount) {
        for (int i = 0; i < CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE; ++i) {
            ingredients[i] = ingr[i];
        }
    }
};

// 背包结构
struct Inventory {
    Item items[INVENTORY_SIZE];
    Item hotbar[HOTBAR_SIZE];
    Item craftingGrid[CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE];
    Item craftingResult;
    int selectedHotbarSlot;
    bool showInventory;
    bool showCrafting;

    Inventory() : selectedHotbarSlot(0), showInventory(false), showCrafting(false) {
        // 初始化背包
        for (int i = 0; i < INVENTORY_SIZE; ++i) {
            items[i] = Item();
        }
        
        // 初始化快捷栏
        hotbar[0] = Item(GRASS_BLOCK, 64);
        hotbar[1] = Item(DIRT_BLOCK, 64);
        hotbar[2] = Item(STONE_BLOCK, 64);
        hotbar[3] = Item(WOOD_BLOCK, 64);
        hotbar[4] = Item(LEAVES_BLOCK, 64);
        
        // 初始化合成网格
        for (int i = 0; i < CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE; ++i) {
            craftingGrid[i] = Item();
        }
        
        craftingResult = Item();
    }

    // 添加物品
    bool addItem(Item item) {
        // 尝试堆叠已有物品
        for (int i = 0; i < INVENTORY_SIZE; ++i) {
            if (items[i].type == item.type && items[i].count < 64) {
                items[i].count += item.count;
                if (items[i].count > 64) {
                    item.count = items[i].count - 64;
                    items[i].count = 64;
                } else {
                    return true;
                }
            }
        }
        
        // 寻找空槽位
        for (int i = 0; i < INVENTORY_SIZE; ++i) {
            if (items[i].isEmpty()) {
                items[i] = item;
                return true;
            }
        }
        
        return false; // 背包已满
    }

    // 切换选中的快捷栏物品
    void selectNextHotbarSlot() {
        selectedHotbarSlot = (selectedHotbarSlot + 1) % HOTBAR_SIZE;
    }
    
    // 切换选中的快捷栏物品（反向）
    void selectPreviousHotbarSlot() {
        selectedHotbarSlot = (selectedHotbarSlot - 1 + HOTBAR_SIZE) % HOTBAR_SIZE;
    }

    // 获取当前选中的物品
    Item getSelectedItem() const {
        return hotbar[selectedHotbarSlot];
    }
    
    // 检查合成配方
    void checkCraftingRecipes(const vector<Recipe>& recipes) {
        craftingResult = Item();
        
        for (const auto& recipe : recipes) {
            bool match = true;
            
            // 检查合成网格中的物品是否与配方匹配
            for (int i = 0; i < CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE; ++i) {
                if (craftingGrid[i].type != recipe.ingredients[i]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                craftingResult = Item(recipe.result, recipe.resultCount);
                break;
            }
        }
    }
    
    // 执行合成
    bool craftItem() {
        if (craftingResult.isEmpty()) return false;
        
        // 添加合成结果到背包
        if (!addItem(craftingResult)) return false;
        
        // 消耗合成材料
        for (int i = 0; i < CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE; ++i) {
            if (!craftingGrid[i].isEmpty()) {
                --craftingGrid[i].count;
                if (craftingGrid[i].count <= 0) {
                    craftingGrid[i] = Item();
                }
            }
        }
        
        return true;
    }
};

// 世界数据
vector<vector<vector<Block>>> world(WORLD_SIZE_X, 
    vector<vector<Block>>(WORLD_SIZE_Y, 
        vector<Block>(WORLD_SIZE_Z)));

// 玩家数据
float playerX = WORLD_SIZE_X / 2.0f;
float playerY = 2.0f;
float playerZ = WORLD_SIZE_Z / 2.0f;
float playerRotX = 0.0f;
float playerRotY = 0.0f;
Inventory playerInventory;
vector<Recipe> craftingRecipes;

// 方块类型名称映射
const char* blockNames[] = {
    "Air", "Grass", "Dirt", "Stone", "Wood", "Leaves", 
    "Sand", "Water", "Coal", "Iron", "Gold", "Diamond"
};

// 物品类型名称映射
const char* itemNames[] = {
    "Air", "Grass Block", "Dirt Block", "Stone Block", "Wood Block", "Leaves Block", 
    "Sand Block", "Water Block", "Coal Block", "Iron Block", "Gold Block", "Diamond Block",
    "Wooden Pickaxe", "Stone Pickaxe", "Iron Pickaxe", "Golden Pickaxe", "Diamond Pickaxe",
    "Wooden Axe", "Stone Axe", "Iron Axe", "Golden Axe", "Diamond Axe",
    "Wooden Shovel", "Stone Shovel", "Iron Shovel", "Golden Shovel", "Diamond Shovel"
};

// 方块纹理颜色映射
void setBlockColor(BlockType type) {
    switch (type) {
        case GRASS:
            glColor3f(0.2f, 0.8f, 0.2f); // 绿色
            break;
        case DIRT:
            glColor3f(0.5f, 0.3f, 0.0f); // 棕色
            break;
        case STONE:
            glColor3f(0.6f, 0.6f, 0.6f); // 灰色
            break;
        case WOOD:
            glColor3f(0.6f, 0.3f, 0.0f); // 深棕色
            break;
        case LEAVES:
            glColor3f(0.0f, 0.6f, 0.0f); // 深绿色
            break;
        case SAND:
            glColor3f(0.9f, 0.9f, 0.6f); // 沙色
            break;
        case WATER:
            glColor4f(0.0f, 0.4f, 0.8f,0.5f); // 半透明蓝色
            break;
        case COAL:
            glColor3f(0.1f, 0.1f, 0.1f); // 黑色
            break;
        case IRON:
            glColor3f(0.7f, 0.7f, 0.7f); // 浅灰色
            break;
        case GOLD:
            glColor3f(1.0f, 0.8f, 0.0f); // 金色
            break;
        case DIAMOND:
            glColor3f(0.0f, 0.8f, 0.8f); // 青色
            break;
        default:
            glColor3f(1.0f, 1.0f, 1.0f); // 白色
    }
}

// 物品纹理颜色映射
void setItemColor(ItemType type) {
    switch (type) {
        case GRASS_BLOCK:
            glColor3f(0.2f, 0.8f, 0.2f); // 绿色
            break;
        case DIRT_BLOCK:
            glColor3f(0.5f, 0.3f, 0.0f); // 棕色
            break;
        case STONE_BLOCK:
            glColor3f(0.6f, 0.6f, 0.6f); // 灰色
            break;
        case WOOD_BLOCK:
            glColor3f(0.6f, 0.3f, 0.0f); // 深棕色
            break;
        case LEAVES_BLOCK:
            glColor3f(0.0f, 0.6f, 0.0f); // 深绿色
            break;
        case SAND_BLOCK:
            glColor3f(0.9f, 0.9f, 0.6f); // 沙色
            break;
        case WATER_BLOCK:
            glColor4f(0.0f, 0.4f, 0.8f,0.5f); // 半透明蓝色
            break;
        case COAL_BLOCK:
            glColor3f(0.1f, 0.1f, 0.1f); // 黑色
            break;
        case IRON_BLOCK:
            glColor3f(0.7f, 0.7f, 0.7f); // 浅灰色
            break;
        case GOLD_BLOCK:
            glColor3f(1.0f, 0.8f, 0.0f); // 金色
            break;
        case DIAMOND_BLOCK:
            glColor3f(0.0f, 0.8f, 0.8f); // 青色
            break;
        case WOODEN_PICKAXE:
            glColor3f(0.6f, 0.3f, 0.0f); // 木色
            break;
        case STONE_PICKAXE:
            glColor3f(0.6f, 0.6f, 0.6f); // 灰色
            break;
        case IRON_PICKAXE:
            glColor3f(0.8f, 0.8f, 0.8f); // 银色
            break;
        case GOLDEN_PICKAXE:
            glColor3f(1.0f, 0.8f, 0.0f); // 金色
            break;
        case DIAMOND_PICKAXE:
            glColor3f(0.0f, 0.8f, 0.8f); // 青色
            break;
        case WOODEN_AXE:
            glColor3f(0.6f, 0.3f, 0.0f); // 木色
            break;
        case STONE_AXE:
            glColor3f(0.6f, 0.6f, 0.6f); // 灰色
            break;
        case IRON_AXE:
            glColor3f(0.8f, 0.8f, 0.8f); // 银色
            break;
        case GOLDEN_AXE:
            glColor3f(1.0f, 0.8f, 0.0f); // 金色
            break;
        case DIAMOND_AXE:
            glColor3f(0.0f, 0.8f, 0.8f); // 青色
            break;
        case WOODEN_SHOVEL:
            glColor3f(0.6f, 0.3f, 0.0f); // 木色
            break;
        case STONE_SHOVEL:
            glColor3f(0.6f, 0.6f, 0.6f); // 灰色
            break;
        case IRON_SHOVEL:
            glColor3f(0.8f, 0.8f, 0.8f); // 银色
            break;
        case GOLDEN_SHOVEL:
            glColor3f(1.0f, 0.8f, 0.0f); // 金色
            break;
        case DIAMOND_SHOVEL:
            glColor3f(0.0f, 0.8f, 0.8f); // 青色
            break;
        default:
            glColor3f(1.0f, 1.0f, 1.0f); // 白色
    }
}

// 绘制一个方块
void drawBlock(int x, int y, int z) {
    Block block = world[x][y][z];
    if (!block.visible) return;

    bool isWater = (block.type == WATER);
    if (isWater) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    setBlockColor(block.type);
    glPushMatrix();
    glTranslatef(x, y, z);
    // 前面
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 1);
    glVertex3f(1, 0, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(0, 1, 1);
    glEnd();
    // 后面
    glBegin(GL_QUADS);
    glVertex3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 1, 0);
    glVertex3f(1, 1, 0);
    glEnd();

    // 顶面
    glBegin(GL_QUADS);
    glVertex3f(0, 1, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(1, 1, 0);
    glVertex3f(0, 1, 0);
    glEnd();

    // 底面
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);
    glVertex3f(1, 0, 1);
    glVertex3f(0, 0, 1);
    glEnd();

    // 右面
    glBegin(GL_QUADS);
    glVertex3f(1, 0, 1);
    glVertex3f(1, 0, 0);
    glVertex3f(1, 1, 0);
    glVertex3f(1, 1, 1);
    glEnd();

    // 左面
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 1);
    glVertex3f(0, 1, 1);
    glVertex3f(0, 1, 0);
    glEnd();

    glPopMatrix();

    if (isWater) {
        glDisable(GL_BLEND);
    }
}

// 绘制物品图标
void drawItemIcon(const Item& item, float x, float y, float size) {
    if (item.isEmpty()) return;

    glPushMatrix();
    glTranslatef(x, y, 0);

    // 绘制物品背景
    glColor4f(0.1f, 0.1f, 0.1f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(size, 0);
    glVertex2f(size, size);
    glVertex2f(0, size);
    glEnd();

    // 绘制物品
    setItemColor(item.type);
    glBegin(GL_QUADS);
    glVertex2f(5, 5);
    glVertex2f(size - 5, 5);
    glVertex2f(size - 5, size - 5);
    glVertex2f(5, size - 5);
    glEnd();

    // 如果是工具，绘制工具图标
    if (item.type >= WOODEN_PICKAXE && item.type <= DIAMOND_SHOVEL) {
        glColor3f(0.0f, 0.0f, 0.0f);
        float toolSize = size / 3;
        
        // 镐子形状
        if (item.type == WOODEN_PICKAXE || item.type == STONE_PICKAXE || 
            item.type == IRON_PICKAXE || item.type == GOLDEN_PICKAXE || 
            item.type == DIAMOND_PICKAXE) {
            glBegin(GL_QUADS);
            glVertex2f(size/2 - toolSize/2, size/2 - toolSize/2);
            glVertex2f(size/2 + toolSize/2, size/2 - toolSize/2);
            glVertex2f(size/2 + toolSize/2, size/2 + toolSize/2);
            glVertex2f(size/2 - toolSize/2, size/2 + toolSize/2);
            glEnd();
            
            glBegin(GL_QUADS);
            glVertex2f(size/2 - toolSize/6, size/2 - toolSize/2 - toolSize/3);
            glVertex2f(size/2 + toolSize/6, size/2 - toolSize/2 - toolSize/3);
            glVertex2f(size/2 + toolSize/6, size/2 + toolSize/2 + toolSize/3);
            glVertex2f(size/2 - toolSize/6, size/2 + toolSize/2 + toolSize/3);
            glEnd();
        }
        // 斧头形状
        else if (item.type == WOODEN_AXE || item.type == STONE_AXE || 
                 item.type == IRON_AXE || item.type == GOLDEN_AXE || 
                 item.type == DIAMOND_AXE) {
            glBegin(GL_QUADS);
            glVertex2f(size/2 - toolSize/2, size/2 - toolSize/2);
            glVertex2f(size/2 + toolSize/2, size/2 - toolSize/2);
            glVertex2f(size/2 + toolSize/2, size/2 + toolSize/2);
            glVertex2f(size/2 - toolSize/2, size/2 + toolSize/2);
            glEnd();
            
            glBegin(GL_QUADS);
            glVertex2f(size/2 + toolSize/2, size/2 - toolSize/2);
            glVertex2f(size/2 + toolSize/2 + toolSize/3, size/2);
            glVertex2f(size/2 + toolSize/2, size/2 + toolSize/2);
            glVertex2f(size/2 + toolSize/6, size/2 + toolSize/3);
            glEnd();
        }
        // 铲子形状
        else if (item.type == WOODEN_SHOVEL || item.type == STONE_SHOVEL || 
                 item.type == IRON_SHOVEL || item.type == GOLDEN_SHOVEL || 
                 item.type == DIAMOND_SHOVEL) {
            glBegin(GL_TRIANGLES);
            glVertex2f(size/2, size/2 - toolSize/2);
            glVertex2f(size/2 + toolSize/2, size/2 + toolSize/2);
            glVertex2f(size/2 - toolSize/2, size/2 + toolSize/2);
            glEnd();
            
            glBegin(GL_QUADS);
            glVertex2f(size/2 - toolSize/6, size/2 - toolSize/2 - toolSize/3);
            glVertex2f(size/2 + toolSize/6, size/2 - toolSize/2 - toolSize/3);
            glVertex2f(size/2 + toolSize/6, size/2 - toolSize/2);
            glVertex2f(size/2 - toolSize/6, size/2 - toolSize/2);
            glEnd();
        }
    }

    // 绘制物品数量
    if (item.count > 1 && item.type < WOODEN_PICKAXE) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(size - 15, size - 5);
        
        string countStr = to_string(item.count);
        for (char c : countStr) {
            glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
        }
    }

    glPopMatrix();
}

// 绘制背包界面
void drawInventory() {
    if (!playerInventory.showInventory && !playerInventory.showCrafting) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // 绘制背景
    glColor4f(0.6f, 0.6f, 0.6f,0.7f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(800, 0);
    glVertex2f(800, 600);
    glVertex2f(0, 600);
    glEnd();

    float slotSize = 40.0f;
    float margin = 20.0f;
    float startX = (800 - (HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * margin)) / 2;
    float startY = 100.0f;

    // 绘制背包标题
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(400 - 40, 600 - 50);
    const char* title = playerInventory.showCrafting ? "Crafting" : "Inventory";
    for (const char* c = title; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // 绘制合成界面
    if (playerInventory.showCrafting) {
        float craftingStartX = 400 - (CRAFTING_GRID_SIZE * slotSize + (CRAFTING_GRID_SIZE - 1) * margin) / 2;
        float craftingStartY = 300;

        // 绘制合成网格
        for (int i = 0; i < CRAFTING_GRID_SIZE; ++i) {
            for (int j = 0; j < CRAFTING_GRID_SIZE; ++j) {
                int index = i * CRAFTING_GRID_SIZE + j;
                float x = craftingStartX + j * (slotSize + margin);
                float y = craftingStartY + i * (slotSize + margin);

                // 绘制槽位边框
                glColor3f(0.5f, 0.5f, 0.5f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(x, y);
                glVertex2f(x + slotSize, y);
                glVertex2f(x + slotSize, y + slotSize);
                glVertex2f(x, y + slotSize);
                glEnd();

                // 绘制物品
                drawItemIcon(playerInventory.craftingGrid[index], x, y, slotSize);
            }
        }

        // 绘制合成结果槽位
        float resultX = craftingStartX + (CRAFTING_GRID_SIZE + 1) * (slotSize + margin);
        float resultY = craftingStartY + slotSize;

        // 绘制结果槽位边框
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(resultX, resultY);
        glVertex2f(resultX + slotSize, resultY);
        glVertex2f(resultX + slotSize, resultY + slotSize);
        glVertex2f(resultX, resultY + slotSize);
        glEnd();

        // 绘制结果物品
        drawItemIcon(playerInventory.craftingResult, resultX, resultY, slotSize);

        // 绘制箭头
        glColor3f(1.0f, 1.0f, 1.0f);
        float arrowX = craftingStartX + CRAFTING_GRID_SIZE * (slotSize + margin) + margin / 2;
        float arrowY = craftingStartY + slotSize + slotSize / 2;
        glBegin(GL_TRIANGLES);
        glVertex2f(arrowX, arrowY);
        glVertex2f(arrowX + margin / 2, arrowY + margin / 4);
        glVertex2f(arrowX + margin / 2, arrowY - margin / 4);
        glEnd();
    }

    // 绘制快捷栏
    for (int i = 0; i < HOTBAR_SIZE; ++i) {
        float x = startX + i * (slotSize + margin);
        float y = startY;

        // 绘制选中槽位的高亮
        if (i == playerInventory.selectedHotbarSlot && !playerInventory.showInventory) {
            glColor3f(1.0f, 1.0f, 0.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x - 2, y - 2);
            glVertex2f(x + slotSize + 2, y - 2);
            glVertex2f(x + slotSize + 2, y + slotSize + 2);
            glVertex2f(x - 2, y + slotSize + 2);
            glEnd();
            glLineWidth(1.0f);
        }

        // 绘制槽位边框
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + slotSize, y);
        glVertex2f(x + slotSize, y + slotSize);
        glVertex2f(x, y + slotSize);
        glEnd();

        // 绘制物品
        drawItemIcon(playerInventory.hotbar[i], x, y, slotSize);
    }

    // 绘制主背包
    if (playerInventory.showInventory) {
        startY += slotSize + margin * 2;
        int rows = (INVENTORY_SIZE - HOTBAR_SIZE) / HOTBAR_SIZE;

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < HOTBAR_SIZE; ++col) {
                int index = HOTBAR_SIZE + row * HOTBAR_SIZE + col;
                float x = startX + col * (slotSize + margin);
                float y = startY + row * (slotSize + margin);

                // 绘制槽位边框
                glColor3f(0.5f, 0.5f, 0.5f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(x, y);
                glVertex2f(x + slotSize, y);
                glVertex2f(x + slotSize, y + slotSize);
                glVertex2f(x, y + slotSize);
                glEnd();

                // 绘制物品
                drawItemIcon(playerInventory.items[index - HOTBAR_SIZE], x, y, slotSize);
            }
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 绘制快捷栏
void drawHotbar() {
    if (playerInventory.showInventory || playerInventory.showCrafting) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    float slotSize = 40.0f;
    float margin = 10.0f;
    float startX = (800 - (HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * margin)) / 2;
    float startY = 30.0f;

    // 绘制快捷栏背景
    glColor4f(0.1f, 0.1f, 0.1f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(startX - margin, startY - margin);
    glVertex2f(startX + HOTBAR_SIZE * (slotSize + margin), startY - margin);
    glVertex2f(startX + HOTBAR_SIZE * (slotSize + margin), startY + slotSize + margin);
    glVertex2f(startX - margin, startY + slotSize + margin);
    glEnd();

    // 绘制快捷栏槽位
    for (int i = 0; i < HOTBAR_SIZE; ++i) {
        float x = startX + i * (slotSize + margin);
        float y = startY;

        // 绘制选中槽位的高亮
        if (i == playerInventory.selectedHotbarSlot) {
            glColor3f(1.0f, 1.0f, 0.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x - 2, y - 2);
            glVertex2f(x + slotSize + 2, y - 2);
            glVertex2f(x + slotSize + 2, y + slotSize + 2);
            glVertex2f(x - 2, y + slotSize + 2);
            glEnd();
            glLineWidth(1.0f);
        }

        // 绘制槽位边框
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + slotSize, y);
        glVertex2f(x + slotSize, y + slotSize);
        glVertex2f(x, y + slotSize);
        glEnd();

        // 绘制槽位编号
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(x + 3, y + slotSize - 10);
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '1' + i);

        // 绘制物品
        drawItemIcon(playerInventory.hotbar[i], x, y, slotSize);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 渲染场景
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 设置相机
    gluLookAt(playerX, playerY, playerZ,
              playerX + sin(playerRotY) * cos(playerRotX),
              playerY + sin(playerRotX),
              playerZ + cos(playerRotY) * cos(playerRotX),
              0.0f, 1.0f, 0.0f);

    // 绘制所有方块
    for (int x = 0; x < WORLD_SIZE_X; ++x) {
        for (int y = 0; y < WORLD_SIZE_Y; ++y) {
            for (int z = 0; z < WORLD_SIZE_Z; ++z) {
                drawBlock(x, y, z);
            }
        }
    }

    // 绘制UI
    drawHotbar();
    drawInventory();

    glutSwapBuffers();
}

// 初始化世界
void initWorld() {
    srand(static_cast<unsigned int>(time(nullptr)));

    // 生成地形
    for (int x = 0; x < WORLD_SIZE_X; ++x) {
        for (int z = 0; z < WORLD_SIZE_Z; ++z) {
            // 随机高度
            int height = rand() % 3 + 2;
            
            // 决定地形类型
            bool isSand = (z > WORLD_SIZE_Z * 0.7) || (z < WORLD_SIZE_Z * 0.3);

            // 放置方块
            for (int y = 0; y < height; ++y) {
                if (y == height - 1) {
                    world[x][y][z] = Block(isSand ? SAND : GRASS);
                } else if (y >= height - 3) {
                    world[x][y][z] = Block(isSand ? SAND : DIRT);
                } else {
                    // 随机生成矿石
                    int oreChance = rand() % 100;
                    if (oreChance < 5) {
                        world[x][y][z] = Block(DIAMOND);
                    } else if (oreChance < 15) {
                        world[x][y][z] = Block(GOLD);
                    } else if (oreChance < 30) {
                        world[x][y][z] = Block(IRON);
                    } else if (oreChance < 50) {
                        world[x][y][z] = Block(COAL);
                    } else {
                        world[x][y][z] = Block(STONE);
                    }
                }
            }

            // 随机生成树木
            if (!isSand && rand() % 10 == 0) {
                int treeHeight = rand() % 3 + 4;
                int treeTop = height + treeHeight;

                // 树干
                for (int y = height; y < treeTop; ++y) {
                    if (y < WORLD_SIZE_Y) {
                        world[x][y][z] = Block(WOOD);
                    }
                }

                // 树叶
                for (int dx = -2; dx <= 2; ++dx) {
                    for (int dy = -2; dy <= 1; ++dy) {
                        for (int dz = -2; dz <= 2; ++dz) {
                            int leafX = x + dx;
                            int leafY = treeTop + dy;
                            int leafZ = z + dz;

                            // 确保在世界范围内且不在树干中心
                            if (leafX >= 0 && leafX < WORLD_SIZE_X &&
                                leafY >= 0 && leafY < WORLD_SIZE_Y &&
                                leafZ >= 0 && leafZ < WORLD_SIZE_Z &&
                                !(dx == 0 && dy <= 0 && dz == 0)) {
                                
                                // 圆形树叶
                                if (dx*dx + dy*dy + dz*dz <= 5) {
                                    world[leafX][leafY][leafZ] = Block(LEAVES);
                                }
                            }
                        }
                    }
                }
            }

            // 随机生成水
            if (height <= 1 && !isSand) {
                world[x][1][z] = Block(WATER);
            }
        }
    }
}

// 初始化合成配方
void initCraftingRecipes() {
    // 木镐配方
    ItemType woodenPickaxeIngredients[CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE] = {
        WOOD_BLOCK, WOOD_BLOCK, AIR_ITEM,
        WOOD_BLOCK, AIR_ITEM, AIR_ITEM,
        AIR_ITEM, WOOD_BLOCK, AIR_ITEM
    };
    craftingRecipes.push_back(Recipe(WOODEN_PICKAXE, 1, woodenPickaxeIngredients));

    // 木斧配方
    ItemType woodenAxeIngredients[CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE] = {
        WOOD_BLOCK, WOOD_BLOCK, AIR_ITEM,
        WOOD_BLOCK, AIR_ITEM, AIR_ITEM,
        AIR_ITEM, WOOD_BLOCK, AIR_ITEM
    };
    craftingRecipes.push_back(Recipe(WOODEN_AXE, 1, woodenAxeIngredients));

    // 木铲配方
    ItemType woodenShovelIngredients[CRAFTING_GRID_SIZE * CRAFTING_GRID_SIZE] = {
        WOOD_BLOCK, AIR_ITEM, AIR_ITEM,
        WOOD_BLOCK, AIR_ITEM, AIR_ITEM,
        AIR_ITEM, WOOD_BLOCK, AIR_ITEM
    };
    craftingRecipes.push_back(Recipe(WOODEN_SHOVEL, 1, woodenShovelIngredients));
}

// 调整窗口大小
void reshape(int width, int height) {
    if (height == 0) height = 1;
    float ratio = 1.0f * width / height;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, ratio, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
}

// 键盘控制
void keyboard(unsigned char key, int x, int y) {
    float moveSpeed = 0.5f;

    switch (key) {
        case 'w':
            playerX += sin(playerRotY) * moveSpeed;
            playerZ += cos(playerRotY) * moveSpeed;
            break;
        case 's':
            playerX -= sin(playerRotY) * moveSpeed;
            playerZ -= cos(playerRotY) * moveSpeed;
            break;
        case 'a':
            playerX += cos(playerRotY) * moveSpeed;
            playerZ -= sin(playerRotY) * moveSpeed;
            break;
        case 'd':
            playerX -= cos(playerRotY) * moveSpeed;
            playerZ += sin(playerRotY) * moveSpeed;
            break;
        case ' ':
            playerY += moveSpeed;
            break;
        case 'c':
            playerY -= moveSpeed;
            break;
        case 'e':
            playerInventory.showInventory = !playerInventory.showInventory;
            playerInventory.showCrafting = false;
            break;
        case 'q':
            playerInventory.showCrafting = !playerInventory.showCrafting;
            playerInventory.showInventory = false;
            if (playerInventory.showCrafting) {
                playerInventory.checkCraftingRecipes(craftingRecipes);
            }
            break;
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            playerInventory.selectedHotbarSlot = key - '1';
            break;
        case 27: // ESC键退出
            exit(0);
            break;
    }

    glutPostRedisplay();
}

// 鼠标控制
void mouse(int x, int y) {
    static int lastX = 0, lastY = 0;
    
    if (lastX == 0 && lastY == 0) {
        lastX = x;
        lastY = y;
        return;
    }

    if (playerInventory.showInventory || playerInventory.showCrafting) {
        lastX = x;
        lastY = y;
        return;
    }

    int dx = x - lastX;
    int dy = y - lastY;

    playerRotY += dx * 0.01f;
    playerRotX += dy * 0.01f;

    // 限制仰角
    if (playerRotX > 1.5f) playerRotX = 1.5f;
    if (playerRotX < -1.5f) playerRotX = -1.5f;
    // 将鼠标限制在窗口中心
	glutWarpPointer(960, 540);
    lastX = 960;
    lastY = 540;
    glutPostRedisplay();
}

// 鼠标点击事件
void mouseClick(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        // 处理背包和合成界面的点击
        if (playerInventory.showInventory || playerInventory.showCrafting) {
            float slotSize = 40.0f;
            float margin = 20.0f;
            float startX = (800 - (HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * margin)) / 2;
            float startY = 100.0f;
            
            // 检查是否点击了合成界面
            if (playerInventory.showCrafting) {
                float craftingStartX = 400 - (CRAFTING_GRID_SIZE * slotSize + (CRAFTING_GRID_SIZE - 1) * margin) / 2;
                float craftingStartY = 300;
                
                // 检查合成网格点击
                for (int i = 0; i < CRAFTING_GRID_SIZE; ++i) {
                    for (int j = 0; j < CRAFTING_GRID_SIZE; ++j) {
                        int index = i * CRAFTING_GRID_SIZE + j;
                        float slotX = craftingStartX + j * (slotSize + margin);
                        float slotY = craftingStartY + i * (slotSize + margin);
                        
                        if (x >= slotX && x <= slotX + slotSize && 
                            y >= slotY && y <= slotY + slotSize) {
                            // 从背包中取物品放入合成网格
                            Item selectedItem = playerInventory.getSelectedItem();
                            if (!selectedItem.isEmpty()) {
                                playerInventory.craftingGrid[index] = selectedItem;
                                playerInventory.hotbar[playerInventory.selectedHotbarSlot] = Item();
                                playerInventory.checkCraftingRecipes(craftingRecipes);
                            }
                            return;
                        }
                    }
                }
                
                // 检查结果槽位点击
                float resultX = craftingStartX + (CRAFTING_GRID_SIZE + 1) * (slotSize + margin);
                float resultY = craftingStartY + slotSize;
                
                if (x >= resultX && x <= resultX + slotSize && 
                    y >= resultY && y <= resultY + slotSize) {
                    // 执行合成
                    if (playerInventory.craftItem()) {
                        playerInventory.checkCraftingRecipes(craftingRecipes);
                    }
                    return;
                }
            }
            
            // 检查快捷栏点击
            for (int i = 0; i < HOTBAR_SIZE; ++i) {
                float slotX = startX + i * (slotSize + margin);
                float slotY = startY;
                
                if (x >= slotX && x <= slotX + slotSize && 
                    y >= slotY && y <= slotY + slotSize) {
                    // 切换选中的快捷栏物品
                    playerInventory.selectedHotbarSlot = i;
                    return;
                }
            }
            
            // 检查主背包点击
            if (playerInventory.showInventory) {
                startY += slotSize + margin * 2;
                int rows = (INVENTORY_SIZE - HOTBAR_SIZE) / HOTBAR_SIZE;
                
                for (int row = 0; row < rows; ++row) {
                    for (int col = 0; col < HOTBAR_SIZE; ++col) {
                        int index = HOTBAR_SIZE + row * HOTBAR_SIZE + col;
                        float slotX = startX + col * (slotSize + margin);
                        float slotY = startY + row * (slotSize + margin);
                        
                        if (x >= slotX && x <= slotX + slotSize && 
                            y >= slotY && y <= slotY + slotSize) {
                            // 交换快捷栏和背包物品
                            Item temp = playerInventory.hotbar[playerInventory.selectedHotbarSlot];
                            playerInventory.hotbar[playerInventory.selectedHotbarSlot] = playerInventory.items[index - HOTBAR_SIZE];
                            playerInventory.items[index - HOTBAR_SIZE] = temp;
                            return;
                        }
                    }
                }
            }
            
            return;
        }

        // 游戏世界中的方块交互
        float rayLength = 5.0f;
        float step = 0.1f;
        float rayX = playerX;
        float rayY = playerY;
        float rayZ = playerZ;
        
        float dirX = sin(playerRotY) * cos(playerRotX);
        float dirY = sin(playerRotX);
        float dirZ = cos(playerRotY) * cos(playerRotX);
        
        // 归一化方向向量
        float length = sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
        dirX /= length;
        dirY /= length;
        dirZ /= length;
        
        int hitX = 0, hitY = 0, hitZ = 0;
        bool hit = false;
        
        for (float t = 0; t < rayLength; t += step) {
            rayX += dirX * step;
            rayY += dirY * step;
            rayZ += dirZ * step;
            
            int blockX = static_cast<int>(rayX);
            int blockY = static_cast<int>(rayY);
            int blockZ = static_cast<int>(rayZ);
            
            // 检查是否在世界范围内
            if (blockX >= 0 && blockX < WORLD_SIZE_X &&
                blockY >= 0 && blockY < WORLD_SIZE_Y &&
                blockZ >= 0 && blockZ < WORLD_SIZE_Z) {
                
                if (world[blockX][blockY][blockZ].visible) {
                    hitX = blockX;
                    hitY = blockY;
                    hitZ = blockZ;
                    hit = true;
                    break;
                }
            }
        }
        
        if (hit) {
            if (button == GLUT_LEFT_BUTTON) {
                // 破坏方块
                BlockType blockType = world[hitX][hitY][hitZ].type;
                
                // 根据方块类型和工具类型决定是否可以破坏
                Item selectedItem = playerInventory.getSelectedItem();
                bool canBreak = true;
                
                // 这里可以添加工具对不同方块的破坏效率和限制
                
                if (canBreak) {
                    world[hitX][hitY][hitZ] = Block(AIR);
                    
                    // 将破坏的方块转换为对应物品
                    ItemType itemType = AIR_ITEM;
                    switch (blockType) {
                        case GRASS: itemType = GRASS_BLOCK; break;
                        case DIRT: itemType = DIRT_BLOCK; break;
                        case STONE: itemType = STONE_BLOCK; break;
                        case WOOD: itemType = WOOD_BLOCK; break;
                        case LEAVES: itemType = LEAVES_BLOCK; break;
                        case SAND: itemType = SAND_BLOCK; break;
                        case COAL: itemType = COAL_BLOCK; break;
                        case IRON: itemType = IRON_BLOCK; break;
                        case GOLD: itemType = GOLD_BLOCK; break;
                        case DIAMOND: itemType = DIAMOND_BLOCK; break;
                        default: break;
                    }
                    
                    if (itemType != AIR_ITEM) {
                        playerInventory.addItem(Item(itemType));
                    }
                }
            } else if (button == GLUT_RIGHT_BUTTON) {
                // 放置方块或使用工具
                Item selectedItem = playerInventory.getSelectedItem();
                
                // 如果是方块，放置方块
                if (selectedItem.type >= GRASS_BLOCK && selectedItem.type <= DIAMOND_BLOCK) {
                    // 计算放置位置（在射线击中位置的外侧）
                    int placeX = hitX - static_cast<int>(dirX * 1.5f);
                    int placeY = hitY - static_cast<int>(dirY * 1.5f);
                    int placeZ = hitZ - static_cast<int>(dirZ * 1.5f);
                    
                    // 检查是否在世界范围内且为空
                    if (placeX >= 0 && placeX < WORLD_SIZE_X &&
                        placeY >= 0 && placeY < WORLD_SIZE_Y &&
                        placeZ >= 0 && placeZ < WORLD_SIZE_Z &&
                        !world[placeX][placeY][placeZ].visible) {
                        
                        // 将物品类型转换为方块类型
                        BlockType blockType = AIR;
                        switch (selectedItem.type) {
                            case GRASS_BLOCK: blockType = GRASS; break;
                            case DIRT_BLOCK: blockType = DIRT; break;
                            case STONE_BLOCK: blockType = STONE; break;
                            case WOOD_BLOCK: blockType = WOOD; break;
                            case LEAVES_BLOCK: blockType = LEAVES; break;
                            case SAND_BLOCK: blockType = SAND; break;
                            default: break;
                        }
                        
                        if (blockType != AIR) {
                            world[placeX][placeY][placeZ] = Block(blockType);
                            
                            // 消耗物品
                            --selectedItem.count;
                            if (selectedItem.count <= 0) {
                                playerInventory.hotbar[playerInventory.selectedHotbarSlot] = Item();
                            } else {
                                playerInventory.hotbar[playerInventory.selectedHotbarSlot] = selectedItem;
                            }
                        }
                    }
                }
                // 如果是工具，可以添加使用工具的逻辑
            }
        }
    }
    
    glutPostRedisplay();
}

// 初始化OpenGL
void initGL() {
    glClearColor(0.5f, 0.7f, 1.0f, 0.9f); // 天空蓝色背景
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// 主函数
int main(int argc, char** argv) {
	#ifdef _WIN32 
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	#endif
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1920,1080);
    glutCreateWindow("Minecraft 3D");

    initGL();
    initWorld();
    initCraftingRecipes();

    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouse);
    glutMouseFunc(mouseClick);
    glutIdleFunc(renderScene);

    // 隐藏鼠标
    glutSetCursor(GLUT_CURSOR_NONE);
    glutMainLoop();
    return 0;
}    
