#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include<ctime>
using namespace std;
#define MINMATCH_SIZE 3 //��Сƥ�䳤��
#define MAXMATCH_SIZE 255+4 //���ƥ�䳤��
#define LOOKAHEAD_SIZE 32 * 1024 //���ڴ�С
#define MINBUFLEN MAXMATCH_SIZE //����ʣ��δ�����ַ���Сֵ
unsigned short HashTable[LOOKAHEAD_SIZE];   //��ϣ��
unsigned short SameHash[LOOKAHEAD_SIZE];    //�����ϣ��ͻ������
unsigned short HASHMASK = (unsigned short)0b111111111111111;//��ϣ����

/****************************************
Function:  updateBuf()
Parameter: conten�ļ����� buf������
           buflen������ʣ���С
           aheadBufBegin���л�������ʼλ��
           READED_INDEX�Ѵ��ļ��ж�ȡ�ַ���
Return:    None
Description:  ���ļ��ж�ȡ���ݣ����»�����
*****************************************/
void updateBuf(string content, char* buf, unsigned int& buflen, unsigned short& aheadBufBegin, unsigned int& READED_INDEX) {
    //�����л������Ѿ�������LOOKAHEAD_SIZE��С�������
    if (aheadBufBegin >= LOOKAHEAD_SIZE) {
        //������������������LOOKAHEAD_SIZE
        for (int i = 0; i < LOOKAHEAD_SIZE; i++) {
            buf[i] = buf[i + LOOKAHEAD_SIZE];
            buf[i + LOOKAHEAD_SIZE] = 0;
        }
        //���¹�ϣ��
        for (int i = 0; i < LOOKAHEAD_SIZE; i++) {
            HashTable[i] = HashTable[i] >= LOOKAHEAD_SIZE ? HashTable[i] - LOOKAHEAD_SIZE : 0;
            SameHash[i] = SameHash[i] >= LOOKAHEAD_SIZE ? SameHash[i] - LOOKAHEAD_SIZE : 0;
        }
        //�������л�������ʼλ��
        aheadBufBegin -= LOOKAHEAD_SIZE;
        //���ļ��л�������δ��ʱ�򻺳����ж�������
        if (READED_INDEX <= content.size()) {
            unsigned int n = (unsigned int)content.copy(buf + LOOKAHEAD_SIZE, LOOKAHEAD_SIZE, READED_INDEX);
            buflen += n;
            READED_INDEX += n;
        }
    }
}

/****************************************
Function:  hashFunction()
Parameter: hash��ϣֵ  c�����ϣ���ַ�
Return:    None(void)
Description: ��ϣ����
*****************************************/
void hashFunction(unsigned short& hash, unsigned char c) {
    hash = ((hash << 5) ^ c) & HASHMASK;
}

/****************************************
Function:  hashAdd()
Parameter: hash��ϣֵ c�ַ�
           aheadBufBegin���л�������ʼλ��
Return:    n��һ��ƥ�䵽��λ��
Description: ���ַ������ϣ��
*****************************************/
unsigned short hashAdd(unsigned short& hash, unsigned char c, unsigned short aheadBufBegin) {
    hashFunction(hash, c);//�����ϣֵ
    unsigned short n = HashTable[hash];//��ȡ��һ��ƥ��λ��
    //���¹�ϣ��
    SameHash[aheadBufBegin & HASHMASK] = HashTable[hash];
    HashTable[hash] = aheadBufBegin;
    return n;
}

/****************************************
Function:  matchLongest()
Parameter: buf������ matchStart��һ��ƥ��λ��
           aheadBufBegin���л�������ʼλ��
           matchMaxStart���ƥ��λ��
Return:    maxLen���ƥ�䳤��
Description:�ҵ����ƥ�䳤��
*****************************************/
unsigned short matchLongest(char* buf, unsigned short matchStart, unsigned short aheadBufBegin, unsigned short& matchMaxStart, unsigned int buflen) {
    unsigned short len = 0;//��ǰƥ�䳤��
    unsigned short maxLen = 0;//���ƥ�䳤��
    unsigned short maxStart = matchStart;//���ƥ��λ��
    int maxTimes = 300;//���ƥ�����
    //ƥ�䳤�����ƣ����ܳ������ƥ�䳤�Ⱥͻ�����ʣ�೤��
    int lenLimit = MAXMATCH_SIZE > buflen ? buflen : MAXMATCH_SIZE;
    while (matchStart > 0 && maxTimes--) {
        char* p = buf + aheadBufBegin;
        char* q = buf + matchStart;
        char* end = q + lenLimit > p ? p : q + lenLimit;//q���ܳ���p
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
        matchStart = SameHash[matchStart & HASHMASK];//������ǰƥ��
    }
    matchMaxStart = aheadBufBegin - maxStart;//����ƫ����
    return maxLen;
}

/****************************************
Function:  compress()
Parameter: finName�����ļ���
Return:    ���гɹ���ʧ�ܱ�־
Description:ѹ���ļ�
*****************************************/
int compress(char* finName, char* foutName) {
    //�������ļ������ļ�ȫ������string�ַ���
    ifstream fin(finName, ios::binary);
    if (!fin) {
        cerr << "\n\t������ʾ��Can not open the input file!\n" << endl;
        return 0;
    }
    istreambuf_iterator<char> beg(fin), end;// ���������ļ�ָ�룬ָ��ʼ�ͽ�������char(һ�ֽ�)Ϊ����
    string content(beg, end);               // ���ļ�ȫ������string�ַ���
    fin.close();                            // �ر��ļ����

    //������ļ�
    ofstream fout(foutName, ios::binary);
    if (!fout) {
        cerr << "\n\t������ʾ��Can not open the output file!\n" << endl;
        return 0;
    }

    //��־λ
    unsigned char flagData = 0;//8bit��־λ����
    unsigned char flagLen = 0;//��ǰ��־���ݳ���
    string flagStr = "";//��¼���б�־����

    //�������ڻ�����
    char* buf = new(nothrow) char[2 * LOOKAHEAD_SIZE]; //���建������64KB)
    if (buf == NULL) {
        cerr << "\n\t������ʾ��û�����뵽�㹻���ڴ�ռ䣡\n" << endl;
        return 0;
    }
    unsigned int READED_INDEX = 0;   //��ǰ�Ѵ�content�ж�ȡ���ֽ���
    unsigned int compressSize = 0;   //��ѹ���ļ���С
    READED_INDEX += (unsigned int)content.copy(buf, 2 * LOOKAHEAD_SIZE, READED_INDEX);//�򻺳����ж�������
    unsigned int buflen = READED_INDEX;  //��ǰ�������д������ֽ���
    unsigned short aheadBufBegin = 0;    //���л�������ʼλ��

    unsigned short hash = 0; //��ϣֵ
    for (int i = 0; i < MINMATCH_SIZE - 1; i++) {
        hashFunction(hash, buf[i]);
    }//��ǰ����2���ַ�������������������ַ�ʱ�Ĺ�ϣֵ��Ϊ�Ե�һ���ַ���ͷ�������ַ��ַ����Ĺ�ϣֵ

    unsigned short matchStart = 0;      //ƥ�䵽�����л��������ַ���ͬ��λ������
    unsigned short matchMaxLen = 0;     //�ڲ��һ�����ƥ�䵽����󳤶�
    unsigned short matchMaxStart = 0;   //�ڲ��һ�����ƥ�����󳤶ȶ�Ӧ��ƫ����

    clock_t tStart = clock();
    //��������ʣ�೤�Ȳ�Ϊ0ʱִ��
    while (buflen) {
        matchMaxLen = 0;
        matchMaxStart = 0;
        //��aheadBuffBegin��Ӧ�����������ַ����뵽��ϣ�б��У������ص�һ��ƥ�䵽��λ��
        matchStart = hashAdd(hash, buf[aheadBufBegin + 2], aheadBufBegin);

        //�����ҵ�ƥ��ʱ����matchStart��Ϊ0��������ǰѰ���ƥ��
        if (matchStart != 0) {
            matchMaxLen = matchLongest(buf, matchStart, aheadBufBegin, matchMaxStart, buflen);
        }

        //���ƥ�䳤�ȴ�����С���Ƴ���(3)
        if (matchMaxLen > MINMATCH_SIZE) {
            //д��(ƫ�ƾ��룬ƥ�䳤��)
            fout << unsigned char(matchMaxLen - 4);
            fout.write((char*)&matchMaxStart, sizeof(unsigned short));
            compressSize += 3;
            //���±�־λ(1)
            flagData <<= 1;
            flagData |= 1;
            flagLen++;
            //����ʣ�໺��������
            buflen -= matchMaxLen;

            //��ƥ�䵽���ַ������뵽��ϣ�б���
            while (--matchMaxLen) {
                aheadBufBegin++;
                matchStart = hashAdd(hash, buf[aheadBufBegin + 2], aheadBufBegin);
            }
            aheadBufBegin++;
        }

        //���ƥ�䳤��С����С���Ƴ���(3)��д�뵥���ַ�
        else {
            fout << (char)buf[aheadBufBegin];
            compressSize += 1;
            //���±�־λ(0)
            flagData <<= 1;
            flagLen++;
            aheadBufBegin++;
            buflen--;
        }
        //���ۼ�8����־λʱ�����뵽flagStr��
        if (flagLen == 8) {
            flagStr += flagData;
            flagData = 0;
            flagLen = 0;
        }
        //��ʣ�໺����С�����Ƴ���ʱ�����»�����
        if (buflen <= MINBUFLEN) {
            updateBuf(content, buf, buflen, aheadBufBegin, READED_INDEX);
        }          
    }

    delete[]buf;//��ʱ�ͷŶ�̬������ڴ�ռ�

    //ʣ��ı�־λ����8λ�����뵽flagStr��
    if (flagLen > 0 && flagLen < 8) {
        flagData <<= (8 - flagLen);
        flagStr += flagData;
    }

    //����־λ����д���ļ���β
    fout << flagStr;
    unsigned int flagSize = (unsigned int)flagStr.size();
    unsigned int fileSize = (unsigned int)content.size();
    //����־�����ֽ�����ԭ�ļ��ֽ���д���ļ���β
    fout.write((char*)&flagSize, sizeof(flagSize));
    fout.write((char*)&fileSize, sizeof(fileSize));
    compressSize += flagSize + sizeof(flagSize) + sizeof(fileSize);
    //�ر��ļ�
    fout.close();
    cout << "**************************************************" << endl;
    cout << "\tѹ�����ļ�·���� " << foutName << endl;
    cout << "\tԭ�ļ���С��     " << content.size() << "�ֽ�" << endl;
    cout << "\tѹ�����ļ���С�� " << compressSize << "�ֽ�" << endl;
    cout << "\tѹ���ʣ�         " << (double)compressSize / content.size() * 100 << "%" << endl;
    cout << "\t�˴�ѹ����ʱ��   " << clock() - tStart << "ms" << endl;
    cout << "**************************************************" << endl;
    return 1;
}

/****************************************
Function:  decompress()
Parameter: finName�����ļ���
Return:    ���гɹ���ʧ�ܱ�־
Description:��ѹ�ļ�
*****************************************/
int decompress(char* finName, char* foutName) {
    //�������ļ�,������������
    ifstream fin(finName, ios::binary);
    if (!fin) {
        cerr << "\n\t������ʾ��Can not open the input file!\n" << endl;
        return 0;
    }
    //�������ļ�,���������־λ
    ifstream finFlag(finName, ios::binary);
    if (!finFlag) {
        cerr << "\n\t������ʾ��Can not open the input file!\n" << endl;
        return 0;
    }

    //������ļ�
    ofstream fout(foutName, ios::binary);
    if (!fout) {
        cerr << "\n\t������ʾ��Can not open the output file!\n" << endl;
        return 0;
    }
    ifstream foutIn(foutName, ios::binary);
    if (!foutIn) {
        cerr << "\n\t������ʾ��Can not open the output file!\n" << endl;
        return 0;
    }

    //����ԭ�ļ��ֽ����ͱ�־�����ֽ���
    unsigned int fileSize = 0;
    unsigned int flagSize = 0;
    finFlag.seekg(-int(sizeof(fileSize) + sizeof(flagSize)), ios::end);
    finFlag.read((char*)&flagSize, sizeof(flagSize));
    finFlag.read((char*)&fileSize, sizeof(fileSize));

    //finFLag��λ����־��������
    finFlag.seekg(-int(sizeof(fileSize) + sizeof(flagSize) + flagSize), ios::end);
    unsigned char flagLen = 0;
    unsigned char flagData = 0;

    //��¼�ѽ�ѹ�ֽ���
    unsigned int READED_INDEX = 0;

    clock_t tStart = clock();
    //���ѽ�ѹ�ֽ�������ԭ�ļ���Сʱִ��
    while (READED_INDEX < fileSize) {
        //��ȡ8λ��־λ
        if (flagLen == 0) {
            flagData = finFlag.get();
            flagLen = 8;
        }

        //����־λΪ1ʱ
        if (flagData & 0x80) {
            //����ƥ�䳤�Ⱥ�ƫ�ƾ���
            unsigned short matchLen = fin.get() + 4;
            unsigned short matchStart = 0;
            fin.read((char*)&matchStart, sizeof(matchStart));

            READED_INDEX += matchLen;

            fout.flush();//ˢ�»�����
            foutIn.seekg(0 - (int)matchStart, ios::end);//��λ��ƥ��λ��
            //��ѹƥ�䳤�ȸ��ֽ�
            while (matchLen--) {
                fout.put(foutIn.get());
            }
        }
        //����־λΪ0ʱ
        else {
            fout.put(fin.get());
            READED_INDEX++;
        }
        //��ȡ��һ����־λ
        flagData <<= 1;
        flagLen--;
    }
    cout << "**************************************************" << endl;
    cout << "\t��ѹ���ļ�·���� " << foutName << endl;
    cout << "\t��ѹ���ļ���С�� " << fileSize << "�ֽ�" << endl;
    cout << "\t�˴ν�ѹ��ʱ��   " << clock() - tStart << "ms" << endl;
    cout << "**************************************************" << endl;
    fin.close();
    finFlag.close();
    fout.close();
    foutIn.close();
    return 1;
}

//������
int main(int argc, char* argv[]) {
    cout << "\n----------��ӭʹ��Zipper----------\n" << endl;
    if (argc != 4) {
        cerr << " ��������ȷ��ָ�" << endl;
        cout << "**************************************************" << endl;
        cout << "    ָ���" << endl;
        cout << "            zip   �����ļ���  ����ļ���" << endl;
        cout << "           unzip  �����ļ���  ����ļ���" << endl;
        cout << "**************************************************" << endl;
        return -1;
    }

    if (!strcmp(argv[1], "zip")) {
        cout << "  ѹ �� �� ........." << endl;
        int flag = compress(argv[2], argv[3]);
        if (flag) {
            cout << "  ѹ*��*��*����" << endl;
        }
        else {
            cout << "  ѹ*��*ʧ*�ܣ���鿴������ʾ��" << endl;
        }
    }
    else if (!strcmp(argv[1], "unzip")) {
        cout << "  �� ѹ �� ........." << endl;
        int flag = decompress(argv[2], argv[3]);
        if (flag) {
            cout << "  ��*ѹ*��*����" << endl;
        }
        else {
            cout << "  ��*ѹ*ʧ*�ܣ���鿴������ʾ��" << endl;
        }
    }
    else {
        cerr << " ��������ȷ��ָ�" << endl;
        cout << "*******************************************************" << endl;
        cout << "    ָ���" << endl;
        cout << "            zip   �����ļ���  ����ļ���(��ʡ��)" << endl;
        cout << "           unzip  �����ļ���  ����ļ���(��ʡ��)" << endl;
        cout << "*******************************************************" << endl;
        return -1;
    }
    return 0;
}