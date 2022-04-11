#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include<ctime>
using namespace std;
#define MINMATCH_SIZE 3 //最小匹配长度
#define MAXMATCH_SIZE 255+4 //最大匹配长度
#define LOOKAHEAD_SIZE 32 * 1024 //窗口大小
#define MINBUFLEN MAXMATCH_SIZE //窗口剩余未处理字符最小值
unsigned short HashTable[LOOKAHEAD_SIZE];   //哈希表
unsigned short SameHash[LOOKAHEAD_SIZE];    //处理哈希冲突的链表
unsigned short HASHMASK = (unsigned short)0b111111111111111;//哈希掩码

/****************************************
Function:  updateBuf()
Parameter: conten文件内容 buf缓冲区
           buflen缓冲区剩余大小
           aheadBufBegin先行缓冲区开始位置
           READED_INDEX已从文件中读取字符数
Return:    None
Description:  从文件中读取数据，更新缓冲区
*****************************************/
void updateBuf(string content, char* buf, unsigned int& buflen, unsigned short& aheadBufBegin, unsigned int& READED_INDEX) {
    //当先行缓冲区已经滑过了LOOKAHEAD_SIZE大小，则更新
    if (aheadBufBegin >= LOOKAHEAD_SIZE) {
        //将缓冲区数据向左移LOOKAHEAD_SIZE
        for (int i = 0; i < LOOKAHEAD_SIZE; i++) {
            buf[i] = buf[i + LOOKAHEAD_SIZE];
            buf[i + LOOKAHEAD_SIZE] = 0;
        }
        //更新哈希表
        for (int i = 0; i < LOOKAHEAD_SIZE; i++) {
            HashTable[i] = HashTable[i] >= LOOKAHEAD_SIZE ? HashTable[i] - LOOKAHEAD_SIZE : 0;
            SameHash[i] = SameHash[i] >= LOOKAHEAD_SIZE ? SameHash[i] - LOOKAHEAD_SIZE : 0;
        }
        //更新先行缓冲区开始位置
        aheadBufBegin -= LOOKAHEAD_SIZE;
        //当文件中还有数据未读时向缓冲区中读入数据
        if (READED_INDEX <= content.size()) {
            unsigned int n = (unsigned int)content.copy(buf + LOOKAHEAD_SIZE, LOOKAHEAD_SIZE, READED_INDEX);
            buflen += n;
            READED_INDEX += n;
        }
    }
}

/****************************************
Function:  hashFunction()
Parameter: hash哈希值  c加入哈希表字符
Return:    None(void)
Description: 哈希函数
*****************************************/
void hashFunction(unsigned short& hash, unsigned char c) {
    hash = ((hash << 5) ^ c) & HASHMASK;
}

/****************************************
Function:  hashAdd()
Parameter: hash哈希值 c字符
           aheadBufBegin先行缓冲区开始位置
Return:    n第一个匹配到的位置
Description: 将字符加入哈希表
*****************************************/
unsigned short hashAdd(unsigned short& hash, unsigned char c, unsigned short aheadBufBegin) {
    hashFunction(hash, c);//计算哈希值
    unsigned short n = HashTable[hash];//获取第一个匹配位置
    //更新哈希表
    SameHash[aheadBufBegin & HASHMASK] = HashTable[hash];
    HashTable[hash] = aheadBufBegin;
    return n;
}

/****************************************
Function:  matchLongest()
Parameter: buf缓冲区 matchStart第一个匹配位置
           aheadBufBegin先行缓冲区开始位置
           matchMaxStart最大匹配位置
Return:    maxLen最大匹配长度
Description:找到最大匹配长度
*****************************************/
unsigned short matchLongest(char* buf, unsigned short matchStart, unsigned short aheadBufBegin, unsigned short& matchMaxStart, unsigned int buflen) {
    unsigned short len = 0;//当前匹配长度
    unsigned short maxLen = 0;//最大匹配长度
    unsigned short maxStart = matchStart;//最大匹配位置
    int maxTimes = 300;//最多匹配次数
    //匹配长度限制：不能超过最大匹配长度和缓冲区剩余长度
    int lenLimit = MAXMATCH_SIZE > buflen ? buflen : MAXMATCH_SIZE;
    while (matchStart > 0 && maxTimes--) {
        char* p = buf + aheadBufBegin;
        char* q = buf + matchStart;
        char* end = q + lenLimit > p ? p : q + lenLimit;//q不能超过p
        len = 0;
        while (*p == *q && q < end) {
            len++;
            p++;
            q++;
        }
        if (len > maxLen) {
            maxLen = len;
            maxStart = matchStart;
        }
        matchStart = SameHash[matchStart & HASHMASK];//继续向前匹配
    }
    matchMaxStart = aheadBufBegin - maxStart;//计算偏移量
    return maxLen;
}

/****************************************
Function:  compress()
Parameter: finName输入文件名
Return:    运行成功或失败标志
Description:压缩文件
*****************************************/
int compress(char* finName, char* foutName) {
    //打开输入文件并将文件全部读入string字符串
    ifstream fin(finName, ios::binary);
    if (!fin) {
        cerr << "\n\t错误提示：Can not open the input file!\n" << endl;
        return 0;
    }
    istreambuf_iterator<char> beg(fin), end;// 设置两个文件指针，指向开始和结束，以char(一字节)为步长
    string content(beg, end);               // 将文件全部读入string字符串
    fin.close();                            // 关闭文件句柄

    //打开输出文件
    ofstream fout(foutName, ios::binary);
    if (!fout) {
        cerr << "\n\t错误提示：Can not open the output file!\n" << endl;
        return 0;
    }

    //标志位
    unsigned char flagData = 0;//8bit标志位数据
    unsigned char flagLen = 0;//当前标志数据长度
    string flagStr = "";//记录所有标志数据

    //滑动窗口缓冲区
    char* buf = new(nothrow) char[2 * LOOKAHEAD_SIZE]; //定义缓冲区（64KB)
    if (buf == NULL) {
        cerr << "\n\t错误提示：没有申请到足够的内存空间！\n" << endl;
        return 0;
    }
    unsigned int READED_INDEX = 0;   //当前已从content中读取的字节数
    unsigned int compressSize = 0;   //解压后文件大小
    READED_INDEX += (unsigned int)content.copy(buf, 2 * LOOKAHEAD_SIZE, READED_INDEX);//向缓冲区中读入数据
    unsigned int buflen = READED_INDEX;  //当前缓冲区中待处理字节数
    unsigned short aheadBufBegin = 0;    //先行缓冲区开始位置

    unsigned short hash = 0; //哈希值
    for (int i = 0; i < MINMATCH_SIZE - 1; i++) {
        hashFunction(hash, buf[i]);
    }//提前处理2个字符，这样当处理第三个字符时的哈希值即为以第一个字符开头的三个字符字符串的哈希值

    unsigned short matchStart = 0;      //匹配到与先行缓冲区首字符相同的位置坐标
    unsigned short matchMaxLen = 0;     //在查找缓冲区匹配到的最大长度
    unsigned short matchMaxStart = 0;   //在查找缓冲区匹配的最大长度对应的偏移量

    clock_t tStart = clock();
    //当缓冲区剩余长度不为0时执行
    while (buflen) {
        matchMaxLen = 0;
        matchMaxStart = 0;
        //将aheadBuffBegin对应的连续三个字符插入到哈希列表中，并返回第一个匹配到的位置
        matchStart = hashAdd(hash, buf[aheadBufBegin + 2], aheadBufBegin);

        //当能找到匹配时，即matchStart不为0，继续向前寻找最长匹配
        if (matchStart != 0) {
            matchMaxLen = matchLongest(buf, matchStart, aheadBufBegin, matchMaxStart, buflen);
        }

        //当最长匹配长度大于最小限制长度(3)
        if (matchMaxLen > MINMATCH_SIZE) {
            //写入(偏移距离，匹配长度)
            fout << unsigned char(matchMaxLen - 4);
            fout.write((char*)&matchMaxStart, sizeof(unsigned short));
            compressSize += 3;
            //更新标志位(1)
            flagData <<= 1;
            flagData |= 1;
            flagLen++;
            //更新剩余缓冲区长度
            buflen -= matchMaxLen;

            //将匹配到的字符串加入到哈希列表中
            while (--matchMaxLen) {
                aheadBufBegin++;
                matchStart = hashAdd(hash, buf[aheadBufBegin + 2], aheadBufBegin);
            }
            aheadBufBegin++;
        }

        //当最长匹配长度小于最小限制长度(3)，写入单个字符
        else {
            fout << (char)buf[aheadBufBegin];
            compressSize += 1;
            //更新标志位(0)
            flagData <<= 1;
            flagLen++;
            aheadBufBegin++;
            buflen--;
        }
        //当累计8个标志位时，加入到flagStr中
        if (flagLen == 8) {
            flagStr += flagData;
            flagData = 0;
            flagLen = 0;
        }
        //当剩余缓冲区小于限制长度时，更新缓冲区
        if (buflen <= MINBUFLEN) {
            updateBuf(content, buf, buflen, aheadBufBegin, READED_INDEX);
        }          
    }

    delete[]buf;//及时释放动态申请的内存空间

    //剩余的标志位补齐8位并加入到flagStr中
    if (flagLen > 0 && flagLen < 8) {
        flagData <<= (8 - flagLen);
        flagStr += flagData;
    }

    //将标志位数据写到文件结尾
    fout << flagStr;
    unsigned int flagSize = (unsigned int)flagStr.size();
    unsigned int fileSize = (unsigned int)content.size();
    //将标志数据字节数和原文件字节数写到文件结尾
    fout.write((char*)&flagSize, sizeof(flagSize));
    fout.write((char*)&fileSize, sizeof(fileSize));
    compressSize += flagSize + sizeof(flagSize) + sizeof(fileSize);
    //关闭文件
    fout.close();
    cout << "**************************************************" << endl;
    cout << "\t压缩后文件路径： " << foutName << endl;
    cout << "\t原文件大小：     " << content.size() << "字节" << endl;
    cout << "\t压缩后文件大小： " << compressSize << "字节" << endl;
    cout << "\t压缩率：         " << (double)compressSize / content.size() * 100 << "%" << endl;
    cout << "\t此次压缩用时：   " << clock() - tStart << "ms" << endl;
    cout << "**************************************************" << endl;
    return 1;
}

/****************************************
Function:  decompress()
Parameter: finName输入文件名
Return:    运行成功或失败标志
Description:解压文件
*****************************************/
int decompress(char* finName, char* foutName) {
    //打开输入文件,用于输入数据
    ifstream fin(finName, ios::binary);
    if (!fin) {
        cerr << "\n\t错误提示：Can not open the input file!\n" << endl;
        return 0;
    }
    //打开输入文件,用于输入标志位
    ifstream finFlag(finName, ios::binary);
    if (!finFlag) {
        cerr << "\n\t错误提示：Can not open the input file!\n" << endl;
        return 0;
    }

    //打开输出文件
    ofstream fout(foutName, ios::binary);
    if (!fout) {
        cerr << "\n\t错误提示：Can not open the output file!\n" << endl;
        return 0;
    }
    ifstream foutIn(foutName, ios::binary);
    if (!foutIn) {
        cerr << "\n\t错误提示：Can not open the output file!\n" << endl;
        return 0;
    }

    //读入原文件字节数和标志数据字节数
    unsigned int fileSize = 0;
    unsigned int flagSize = 0;
    finFlag.seekg(-int(sizeof(fileSize) + sizeof(flagSize)), ios::end);
    finFlag.read((char*)&flagSize, sizeof(flagSize));
    finFlag.read((char*)&fileSize, sizeof(fileSize));

    //finFLag定位到标志数据区域
    finFlag.seekg(-int(sizeof(fileSize) + sizeof(flagSize) + flagSize), ios::end);
    unsigned char flagLen = 0;
    unsigned char flagData = 0;

    //记录已解压字节数
    unsigned int READED_INDEX = 0;

    clock_t tStart = clock();
    //当已解压字节数少于原文件大小时执行
    while (READED_INDEX < fileSize) {
        //获取8位标志位
        if (flagLen == 0) {
            flagData = finFlag.get();
            flagLen = 8;
        }

        //当标志位为1时
        if (flagData & 0x80) {
            //读入匹配长度和偏移距离
            unsigned short matchLen = fin.get() + 4;
            unsigned short matchStart = 0;
            fin.read((char*)&matchStart, sizeof(matchStart));

            READED_INDEX += matchLen;

            fout.flush();//刷新缓冲区
            foutIn.seekg(0 - (int)matchStart, ios::end);//定位到匹配位置
            //解压匹配长度个字节
            while (matchLen--) {
                fout.put(foutIn.get());
            }
        }
        //当标志位为0时
        else {
            fout.put(fin.get());
            READED_INDEX++;
        }
        //获取下一个标志位
        flagData <<= 1;
        flagLen--;
    }
    cout << "**************************************************" << endl;
    cout << "\t解压后文件路径： " << foutName << endl;
    cout << "\t解压后文件大小： " << fileSize << "字节" << endl;
    cout << "\t此次解压用时：   " << clock() - tStart << "ms" << endl;
    cout << "**************************************************" << endl;
    fin.close();
    finFlag.close();
    fout.close();
    foutIn.close();
    return 1;
}

//主函数
int main(int argc, char* argv[]) {
    cout << "\n----------欢迎使用Zipper----------\n" << endl;
    if (argc != 4) {
        cerr << " 请输入正确的指令！" << endl;
        cout << "**************************************************" << endl;
        cout << "    指令集：" << endl;
        cout << "            zip   输入文件名  输出文件名" << endl;
        cout << "           unzip  输入文件名  输出文件名" << endl;
        cout << "**************************************************" << endl;
        return -1;
    }

    if (!strcmp(argv[1], "zip")) {
        cout << "  压 缩 中 ........." << endl;
        int flag = compress(argv[2], argv[3]);
        if (flag) {
            cout << "  压*缩*成*功！" << endl;
        }
        else {
            cout << "  压*缩*失*败！请查看错误提示。" << endl;
        }
    }
    else if (!strcmp(argv[1], "unzip")) {
        cout << "  解 压 中 ........." << endl;
        int flag = decompress(argv[2], argv[3]);
        if (flag) {
            cout << "  解*压*成*功！" << endl;
        }
        else {
            cout << "  解*压*失*败！请查看错误提示。" << endl;
        }
    }
    else {
        cerr << " 请输入正确的指令！" << endl;
        cout << "*******************************************************" << endl;
        cout << "    指令集：" << endl;
        cout << "            zip   输入文件名  输出文件名(可省略)" << endl;
        cout << "           unzip  输入文件名  输出文件名(可省略)" << endl;
        cout << "*******************************************************" << endl;
        return -1;
    }
    return 0;
}