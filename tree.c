/**
 * 实验：目录树查看器（仿 Linux tree 命令）
 * 学号：2504020427 姓名：何媛佳
 * 说明：请补全所有标记为 TODO 的函数体，不要修改其他代码。
 * 目录树查看器（仿 Linux tree 命令）
 * 完整实现版本（C语言，左孩子右兄弟二叉树）
 * 编译：gcc -o tree tree.c -std=c99
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// ================== 二叉树结点定义 ==================
typedef struct FileNode {
    char *name;                  // 文件/目录名
    int isDir;                   // 1:目录 0:文件
    struct FileNode *firstChild; // 左孩子：第一个子项
    struct FileNode *nextSibling;// 右兄弟：下一个同层项
} FileNode;

// ================== 函数声明 ==================
FileNode* createNode(const char *name, int isDir);
int cmpNode(const void *a, const void *b);
FileNode* buildTree(const char *path);
void printTree(FileNode *node, const char *prefix, int isLast);
int countNodes(FileNode *root);
int countLeaves(FileNode *root);
int treeHeight(FileNode *root);
void countDirFile(FileNode *root, int *dirs, int *files);
void freeTree(FileNode *root);
char* getBaseName(void);

// ================== 需要补全的函数 ==================

// 创建新结点（分配内存、复制字符串、初始化指针）
FileNode* createNode(const char *name, int isDir) {
    // TODO: 实现
     FileNode *node = (FileNode*)malloc(sizeof(FileNode));
        if (!node) {
            perror("malloc failed");
            return NULL;
        }
        node->name = strdup(name); // 复制字符串
        if (!node->name) {
            perror("strdup failed");
            free(node);
            return NULL;
        }
        node->isDir = isDir;
        node->firstChild = NULL;
        node->nextSibling = NULL;
        return node;
    return NULL;
}

// 比较函数，用于 qsort 对子项按名称排序
int cmpNode(const void *a, const void *b) {
    // TODO: 实现
     const FileNode *nodeA = *(const FileNode**)a;
        const FileNode *nodeB = *(const FileNode**)b;
        return strcmp(nodeA->name, nodeB->name);
    return 0;
}

// 递归构建目录树（核心难点）
FileNode* buildTree(const char *path) {
     DIR *dir = opendir(path);
        if (!dir) {
            // 无法打开目录（可能是文件、无权限或不存在），这里不打印错误，由调用者处理
            return NULL;
        }
    
        // 从路径中提取基本名称作为当前结点名
        const char *basename = strrchr(path, '/');
        if (basename && *(basename + 1) != '\0') {
            // 如果路径包含 '/' 且后面有字符，则取 '/' 之后的部分
            basename++;
        } else if (!basename || *(basename + 1) == '\0') {
            // 如果没有 '/' 或者 '/' 是最后一个字符，则整个路径就是名称（如 "." 或 ".." 或根目录 "/" 的情况）
            // 这里简化处理，对于根目录或当前目录，我们直接用路径本身
            // 但为了显示，我们通常希望得到最后一部分。这里处理一种常见情况：如果路径是 "/"，basename 就指向空。
            // 我们用一个更健壮的方法：如果 basename 为空或指向结束符，就用 path
            if (!basename || *basename == '\0') {
                basename = path;
            }
            // 特殊情况：如果 path 是 "/"，basename 指向 "/"，我们希望显示 "/" 吗？
            // 实际上，我们传入的 path 通常是绝对路径或相对路径，如 "test"。
            // 对于构建树的递归调用，我们会处理子路径，如 "test/sub1"。
            // 当递归到 "test/sub1" 时，basename 会正确指向 "sub1"。
        }
    
        // 创建当前目录结点
        FileNode *currentDir = createNode(basename, 1);
        if (!currentDir) {
            closedir(dir);
            return NULL;
        }
    
        struct dirent *entry;
        FileNode **children = NULL; // 动态数组，存放子结点指针
        int capacity = 10;
        int count = 0;
    
        children = (FileNode**)malloc(capacity * sizeof(FileNode*));
        if (!children) {
            perror("malloc failed");
            freeTree(currentDir);
            closedir(dir);
            return NULL;
        }
    
        while ((entry = readdir(dir)) != NULL) {
            // 跳过 "." 和 ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
    
            // 构造完整路径
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
    
            struct stat st;
            if (stat(fullpath, &st) != 0) {
                // 如果 stat 失败（如符号链接 broken），跳过此项
                continue;
            }
    
            FileNode *childNode = NULL;
            if (S_ISDIR(st.st_mode)) {
                // 递归构建子目录树
                childNode = buildTree(fullpath);
            } else if (S_ISREG(st.st_mode)) {
                // 创建文件结点
                childNode = createNode(entry->d_name, 0);
            } else {
                // 跳过其他类型文件（如符号链接、设备文件等，可选处理）
                continue;
            }
    
            if (childNode) {
                if (count >= capacity) {
                    capacity *= 2;
                    FileNode **new_children = (FileNode**)realloc(children, capacity * sizeof(FileNode*));
                    if (!new_children) {
                        perror("realloc failed");
                        free(childNode);
                        break;
                    }
                    children = new_children;
                }
                children[count++] = childNode;
            }
        }
        closedir(dir);
    
        // 对子结点按名称排序
        if (count > 0) {
            qsort(children, count, sizeof(FileNode*), cmpNode);
    
            // 将排序后的子结点链接成兄弟链表
            currentDir->firstChild = children[0];
            for (int i = 0; i < count - 1; i++) {
                children[i]->nextSibling = children[i + 1];
            }
            children[count - 1]->nextSibling = NULL;
        }
    
        // 释放临时数组
        free(children);
        return currentDir;
    // TODO: 实现
    // 步骤提示：
    // 1. opendir 打开目录，失败返回 NULL
    // 2. 从 path 中提取最后的目录名作为当前结点名（注意处理根目录"/"）
    // 3. 创建当前目录结点
    // 4. 循环 readdir，跳过 "." 和 ".."
    // 5. 拼接完整路径，用 stat 判断类型
    // 6. 若是目录，递归调用 buildTree；若是普通文件，调用 createNode
    // 7. 将得到的子结点存入临时数组
    // 8. 关闭目录
    // 9. 对子结点数组排序（调用 qsort 和 cmpNode）
    // 10. 将排序后的子结点链接成兄弟链表（firstChild 指向第一个，后续 nextSibling）
    // 11. 释放临时数组，返回当前目录结点
    return NULL;
}

// 树形输出（仿 tree 命令）
void printTree(FileNode *node, const char *prefix, int isLast) {
    if (!node) return;
    
        // 打印当前结点
        printf("%s", prefix);
        printf(isLast ? "`-- " : "|-- ");
        printf("%s", node->name);
        if (node->isDir) {
            printf("/");
        }
        printf("\n");
    
        // 如果没有孩子，直接返回
        if (!node->firstChild) {
            return;
        }
    
        // 构建新的前缀
        int prefixLen = strlen(prefix) + 5; // 4 for "    " or "|   ", 1 for '\0'
        char *newPrefix = (char*)malloc(prefixLen);
        if (!newPrefix) {
            perror("malloc failed in printTree");
            return;
        }
        snprintf(newPrefix, prefixLen, "%s%s", prefix, isLast ? "    " : "|   ");
    
        // 遍历所有孩子
        FileNode *child = node->firstChild;
        int childCount = 0;
        FileNode *tmp = child;
        while (tmp) {
            childCount++;
            tmp = tmp->nextSibling;
        }
        int idx = 0;
        while (child) {
            idx++;
            int childIsLast = (idx == childCount);
            printTree(child, newPrefix, childIsLast);
            child = child->nextSibling;
        }
    
        free(newPrefix);
    // TODO: 实现
    // 步骤提示：
    // 1. 如果 node 为空，返回
    // 2. 输出前缀、分支符号（isLast ? "`-- " : "|-- "）、结点名
    // 3. 如果是目录，输出 "/"
    // 4. 换行
    // 5. 如果没有孩子，返回
    // 6. 遍历孩子链表，对每个孩子：
    //     计算新前缀 = prefix + (isLast ? "    " : "|   ")
    //     判断是否为最后一个孩子
    //     递归调用 printTree
}

// 统计二叉树结点总数
int countNodes(FileNode *root) {
    if (!root) return 0;
        return 1 + countNodes(root->firstChild) + countNodes(root->nextSibling);
    // TODO: 实现（递归）
    return 0;
}

// 统计叶子结点数（firstChild == NULL 的结点）
int countLeaves(FileNode *root) {
   if (!root) return 0;
       if (!root->firstChild) {
           // 当前结点是叶子
           return 1 + countLeaves(root->nextSibling);
       } else {
           // 当前结点有孩子，不是叶子
           return countLeaves(root->firstChild) + countLeaves(root->nextSibling);
       }
    // TODO: 实现（递归）
    return 0;
}

// 计算二叉树高度（根深度为1，空树高度为0）
int treeHeight(FileNode *root) {
    if (!root) return 0;
        int childHeight = treeHeight(root->firstChild);
        int siblingHeight = treeHeight(root->nextSibling);
        // 当前结点的高度是：1 + 孩子子树的高度，与兄弟树的高度，两者取较大值
        int height = 1 + childHeight;
        return (height > siblingHeight) ? height : siblingHeight;
    // TODO: 实现（递归）
    return 0;
}

// 统计目录数和文件数（遍历整棵树）
void countDirFile(FileNode *root, int *dirs, int *files) {
      if (!root) return;
        if (root->isDir) {
            (*dirs)++;
        } else {
            (*files)++;
        }
        countDirFile(root->firstChild, dirs, files);
        countDirFile(root->nextSibling, dirs, files);
    // TODO: 实现（递归）
}

// 释放整棵树的内存
void freeTree(FileNode *root) {
    if (!root) return;
        // 后序遍历：先释放孩子，再释放兄弟，最后释放自己
        freeTree(root->firstChild);
        freeTree(root->nextSibling);
        free(root->name);
        free(root);
    // TODO: 实现（递归释放左右子树，最后释放当前结点）
}

// 获取当前工作目录的“基本名称”（用于显示根结点名）
char* getBaseName(void) {
    char *cwd = getcwd(NULL, 0); // 让系统分配缓冲区
        if (!cwd) {
            perror("getcwd");
            return NULL;
        }
        char *basename = strrchr(cwd, '/');
        char *result;
        if (basename && *(basename + 1) != '\0') {
            result = strdup(basename + 1);
        } else {
            result = strdup(cwd);
        }
        free(cwd);
        return result;
    // TODO: 实现
    // 提示：调用 getcwd(NULL,0) 获取绝对路径，提取最后一个 '/' 之后的部分
    // 注意释放 getcwd 分配的内存
    return NULL;
}

int main(int argc, char *argv[]) {
    char targetPath[1024];
    if (argc >= 2) {
        strncpy(targetPath, argv[1], sizeof(targetPath)-1);
        targetPath[sizeof(targetPath)-1] = '\0';
    } else {
        if (getcwd(targetPath, sizeof(targetPath)) == NULL) {
            perror("getcwd");
            return 1;
        }
    }

    int len = strlen(targetPath);
    if (len > 0 && targetPath[len-1] == '/')
        targetPath[len-1] = '\0';

    struct stat st;
    if (stat(targetPath, &st) != 0) {
        perror("stat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "错误: %s 不是目录\n", targetPath);
        return 1;
    }

    FileNode *root = buildTree(targetPath);
    if (!root) {
        fprintf(stderr, "无法构建目录树\n");
        return 1;
    }

    // 输出根目录名
    char *displayName = NULL;
    if (argc >= 2) {
        displayName = root->name;
    } else {
        displayName = getBaseName();
    }
    printf("%s/\n", displayName);
    if (argc < 2) free(displayName);

    FileNode *child = root->firstChild;
    int childCount = 0;
    FileNode *tmp = child;
    while (tmp) { childCount++; tmp = tmp->nextSibling; }
    int idx = 0;
    while (child) {
        int isLast = (++idx == childCount);
        printTree(child, "", isLast);
        child = child->nextSibling;
    }

    int dirs = 0, files = 0;
    countDirFile(root, &dirs, &files);
    printf("\n%d 个目录, %d 个文件\n", dirs, files);
    printf("二叉树结点总数: %d\n", countNodes(root));
    printf("叶子结点数: %d\n", countLeaves(root));
    printf("树的高度: %d\n", treeHeight(root));

    freeTree(root);
    return 0;
}
