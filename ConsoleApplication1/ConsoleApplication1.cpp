// ConsoleApplication1.cpp : �������̨Ӧ�ó������ڵ㡣

#include "stdafx.h"  
#include "SerialPort.h"
#include <process.h>  
#include <iostream>

using namespace std;

/*
	ȫ�ֱ����������궨��
*/
//���ڶ˿ں�
#define PORT 2

/// <summary>
/// �����ŵ�ǰ���ã���λ������ʽ
/// </summary>
UCHAR  CmdReadId[8] = { 0x01, 0x08, 0xA1, 0x20, 0x00, 0x01, 0x00, 0x76 };

/// <summary>
/// ��д������
/// </summary>
UCHAR  Cmd[23] = { 0x01, 0x17, 0xA4, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/// <summary>
/// ����Ǯ������ֵ���ۿ���������ʽ
/// </summary>
UCHAR wallet_OP_Cmd[11] = { 0x01, 0x0B, 0xA6, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/// <summary>
/// ��ѯǮ����������ʽ
/// </summary>
UCHAR queryWallet_Cmd[8] = { 0x01, 0x08, 0xA9, 0x20, 0x00, 0x00, 0x00, 0x00 };


/// <summary>
/// У����
/// </summary>
/// <param name="buf">buf������������</param>
/// <param name="len">len�����ݳ���</param>
void CheckSumOut(UCHAR *buf, UCHAR len)
{
	UCHAR i;
	UCHAR checksum;
	checksum = 0;
	for (i = 0; i < (len - 1); i++)
	{
		checksum ^= buf[i];
	}
	buf[len - 1] = (UCHAR)~checksum;
}

/// <summary>
/// У����
/// </summary>
/// <param name="buf"></param>
/// <param name="len"></param>
/// <returns></returns>
bool CheckSumIn(UCHAR *buf, UCHAR len)
{
	UCHAR i;
	UCHAR checksum;
	checksum = 0;
	for (i = 0; i < (len - 1); i++)
	{
		checksum ^= buf[i];
	}
	if (buf[len - 1] == (UCHAR)~checksum)
	{
		return true;
	}
	return false;
}

/// <summary>
/// �ֽ���ת��Ϊʮ�������ַ�������һ��ʵ�ַ�ʽ 
/// </summary>
/// <param name="sSrc"></param>
/// <param name="sDest"></param>
/// <param name="nSrcLen"></param>
void Hex2Str(const UCHAR *sSrc, UCHAR *sDest, int nSrcLen)
{
	int  i;
	char szTmp[3];

	for (i = 0; i < nSrcLen; i++)
	{
		sprintf_s(szTmp, "%02X", (unsigned char)sSrc[i]);
		memcpy(&sDest[i * 2], szTmp, 2);
	}
	sDest[nSrcLen * 2] = '\0';
	return;
}
/// <summary>
/// ʮ�������ַ���ת��Ϊ�ֽ���  
/// </summary>
/// <param name="source"></param>
/// <param name="dest"></param>
/// <param name="sourceLen"></param>
void HexStrToByte(const UCHAR* source,  UCHAR* dest, int sourceLen)
{
	short i;
	unsigned char highByte, lowByte;

	/*for (int k = 0;k < sourceLen;k++) {
		cout << int(source[k]);
	}*/

	for (i = 0; i < sourceLen; i += 2)
	{
		highByte = toupper(source[i]);
		lowByte = toupper(source[i + 1]);

		if (highByte > 0x39)
			highByte -= 0x37;
		else
			highByte -= 0x30;

		if (lowByte > 0x39)
			lowByte -= 0x37;
		else
			lowByte -= 0x30;

		dest[i / 2] = (highByte << 4) | lowByte;
	}
	return;
}

/// <summary>
/// �����ź�����װ
/// </summary>
/// <param name="argc">���ڶ���*mySerialPort</param>
/// <param name="argv"></param>
/// <returns>none</returns>

void readIC(SerialPort *mySerialPort) {
	UCHAR inbyte;
	UCHAR revdata[32];
	UINT len = 0;
	UINT readbytes;
	CheckSumOut(CmdReadId, CmdReadId[1]);
	mySerialPort->WriteData(CmdReadId, CmdReadId[1]);
	Sleep(200);								// ��ʱ200����ȴ���д���������ݣ���ʱ̫С�����޷��������������ݰ�
	len = mySerialPort->GetBytesInCOM();	//��ȡ���ڻ��������ֽ���
	if (len >= 8)							// �����Ŷ��������ص����ݰ����ȣ�ʧ��Ϊ8�ֽڣ��ɹ�Ϊ12�ֽ�
	{
		readbytes = 0;
		do									// ��ȡ���ڻ���������
		{
			inbyte = 0;
			if (mySerialPort->ReadChar(inbyte) == true)
			{
				revdata[readbytes] = inbyte;
				readbytes++;
			}
		} while (--len);
		if ((revdata[0] = 0x01) && ((revdata[1] == 8) || (revdata[1] == 12)) && (revdata[1] == readbytes) && (revdata[2] == 0x0A1) && (revdata[3] = 0x20))
		{
			bool status = CheckSumIn(revdata, revdata[1]);
			if (status)
			{
				if (revdata[4] == 0x00)
				{
					UCHAR cardtype[5];						//���濨����
					UCHAR id[9];							//���濨��
					Hex2Str(&revdata[5], &cardtype[0], 2);	// ����revdata[5]��ʼ2�ֽ�Ϊ������
					Hex2Str(&revdata[7], &id[0], 4);		// ����revdata[7]��ʼ4�ֽ�Ϊ����
					cout << "��ǰIC������:" << cardtype << "�����ţ�" << id << endl;
				}
				else
				{
					cout << "δ��⵽������IC���õ���д���������ſɶ�����" << endl;
				}
			}
		}
	}
	else
	{
		cout << "������ʱ������������������Ƿ�������" << endl;
		while (len > 0)
		{
			mySerialPort->ReadChar(inbyte);
		}
	}

	return;
}


/// <summary>
/// ��λ�������������λ��
/// </summary>
/// <param name="block">�����Ŀ��</param>
/// <param name="cmd">��������������
///		д��ţ�		1
///		����ţ�		2
///		��ʼ��Ǯ����	3
///		��ѯ��	4
///		Ǯ����ֵ��	5
///		ʹ��Ǯ����	6
///		���÷�����״̬��7
/// </param>
/// /// <param name="mySerialPort">���ڶ���ʵ��</param>
void sendCommand(INT block, INT cmd, SerialPort *mySerialPort) {
	//���ݳ���
	UINT len = 0;
	//���ܵ�����
	UCHAR revdata[100];
	//��ȡ���ֽ���
	UINT readbytes;
	//�������ַ�
	UCHAR inbyte;
	//��������
	UINT i = 0;

	if (block > 0)
	{
		switch (cmd) {
			//д���ݿ�
			//�����ݿ�
		case 1:
		case 2:
			CheckSumOut(Cmd, Cmd[1]);				//����У����
			mySerialPort->WriteData(Cmd, Cmd[1]);  //ͨ�����ڷ��Ͷ����ݿ�ָ�����д��
			Sleep(200);							 // ��ʱ200����ȴ���д���������ݣ���ʱ̫С�����޷��������������ݰ�
			len = mySerialPort->GetBytesInCOM(); //��ȡ���ڻ��������ֽ���
			if (len >= 8)
			{
				readbytes = 0;
				do								// ��ȡ���ڻ���������
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//���ܷ�������
						readbytes++;
					}
				} while (--len);

				//�жϷ��ص����ݰ���ʽ
				if ((revdata[0] == 0x01) && (revdata[1] == 8) && (revdata[1] == readbytes) && (revdata[2] == 0x0A4) && (revdata[3] = 0x20)) //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
				{
					bool status = CheckSumIn(revdata, revdata[1]);  //����У���
					if (status)
					{
						if (revdata[4] == 0x00)						//д���ݿ�ɹ�
						{
							cout << "д���ݵ����ݿ�" << block << "�ɹ���" << endl << endl;
						}
						else										//д���ݿ�ʧ��
						{
							cout << "��,д���ݿ�ʧ��,ʧ��ԭ�����£�" << endl;
							cout << "1. ���IC���Ƿ�����ڶ�д���ĸ�Ӧ����." << endl;
							cout << "2. IC����Ӧ�����������д����д���벻һ��." << endl;
							cout << "3. ��������ݿ�ֵ����IC����������ݿ���ֵ������S50����63�����ݿ�." << endl;
							cout << "4. ������ƿ鲻���Զ���д." << endl;
						}
					}
				}
				if ((revdata[0] == 0x01) && ((revdata[1] == 8) || (revdata[1] == 22)) && (revdata[1] == readbytes) && (revdata[2] == 0xA3) && (revdata[3] = 0x20))//�ж��Ƿ�Ϊ�����ݿ鷵�ص����ݰ�
				{
					bool status = CheckSumIn(revdata, revdata[1]);	//����У���
					if (status)
					{
						if (revdata[4] == 0x00)						//�����ݿ�ɹ�
						{
							UCHAR blockdata[16];
							UCHAR temp[33];
							for (i = 0; i < 16; i++)
							{
								blockdata[i] = revdata[5 + i];		//�������ݵ�����
							}
							Hex2Str(&blockdata[0], &temp[0], 16);	// ���ݿ�����ת��Ϊ�ַ�
							cout << "�����ݿ�ɹ������ݿ�" << block << "����Ϊ��" << temp << endl << endl;
						}
						else										//�����ݿ�ʧ��
						{
							cout << "��,д���ݿ�ʧ��,ʧ��ԭ�����£�" << endl;
							cout << "1. ���IC���Ƿ�����ڶ�д���ĸ�Ӧ����." << endl;
							cout << "2. IC����Ӧ�����������д����д���벻һ��." << endl;
							cout << "3. ��������ݿ�ֵ����IC����������ݿ���ֵ������S50����63�����ݿ�." << endl;
							cout << "4. ������ƿ鲻���Զ���д." << endl;
						}
					}
				}
			}
			else
			{
				cout << "��д����ʱ����������������������Ƿ�������" << endl;
				while (len > 0) //����������������ݣ������������������
				{
					mySerialPort->ReadChar(inbyte);
				}
			}
			break;

		case 3://����Ǯ��
		case 5://Ǯ����ֵ
		case 6://ʹ��Ǯ��
			CheckSumOut(wallet_OP_Cmd, wallet_OP_Cmd[1]);				//����У����
			mySerialPort->WriteData(wallet_OP_Cmd, wallet_OP_Cmd[1]);  //ͨ�����ڷ��Ͷ����ݿ�ָ�����д��
			Sleep(200);							 // ��ʱ200����ȴ���д���������ݣ���ʱ̫С�����޷��������������ݰ�
			len = mySerialPort->GetBytesInCOM(); //��ȡ���ڻ��������ֽ���
			if (len >= 8)
			{
				readbytes = 0;
				do								// ��ȡ���ڻ���������
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//���ܷ�������
						readbytes++;
					}
				} while (--len);

				//�жϷ��ص����ݰ���ʽ
				//��ʼ��ʧ��
				if (revdata[0] == 0x01 && revdata[2] == 0xA6 && revdata[4] == 0x01) {
					cout << "��ʼ��Ǯ��ʧ�ܣ���������´���" << endl;
					return;
				}
				//��ʼ���ɹ������������
				//������ʼ���ɹ�������ѯ���ʧ�ܵ����
				else if ((revdata[0] == 0x01) && (revdata[1] == 0x0A || revdata[1] == 0x08) && (revdata[2] == 0xA6) && (revdata[3] == 0x20)) { //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
					bool status = CheckSumIn(revdata, revdata[1]);  //����У���
					if (status)
					{
						if (revdata[4] == 0x00)						//д���ݿ�ɹ�
						{
							cout << "�����ݿ�" << block << "����Ǯ���ɹ��� ��ǰ���Ϊ��";

							UCHAR revBalance[4] = { 0x00 };
							for (int d = 0;d < 4;d++) {
								revBalance[d] = revdata[5 + d];
							}

							UCHAR balance[9] = {};
							Hex2Str(&revBalance[0], &balance[0], 4);
							for (int k = 0;k < 8;k++) {
								cout << balance[k];
							}
							cout << endl;
						}
						else if (revdata[4] == 0x03) {
							cout << "�����ݿ�" << block << "����Ǯ���ɹ��� ����ѯ���ʧ��" << endl;
						}
						else										//д���ݿ�ʧ��
						{
							cout << "��,д���ݿ�ʧ��,ʧ��ԭ�����£�" << endl;
							cout << "1. ���IC���Ƿ�����ڶ�д���ĸ�Ӧ����." << endl;
							cout << "2. IC����Ӧ�����������д����д���벻һ��." << endl;
							cout << "3. ��������ݿ�ֵ����IC����������ݿ���ֵ������S50����63�����ݿ�." << endl;
							cout << "4. ������ƿ鲻���Զ���д." << endl;
						}
					}
					return;
				}

				//��ֵ
				if (revdata[0] == 0x01 && revdata[2] == 0xA8 && revdata[4] == 0x01) {
					cout << "Ǯ����ֵʧ�ܣ���������³�ֵ" << endl;
					return;
				}
				//��ֵ�ɹ������������
				//������ֵ�ɹ�������ѯ���ʧ�ܵ����
				else if ((revdata[0] == 0x01) && (revdata[1] == 0x0A || revdata[1] == 0x08) && (revdata[2] == 0xA8) && (revdata[3] == 0x20)) { //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
					bool status = CheckSumIn(revdata, revdata[1]);  //����У���
					if (status)
					{
						if (revdata[4] == 0x00)						//д���ݿ�ɹ�
						{
							cout << "�����ݿ�" << block << "Ǯ����ֵ�ɹ��� ��ǰ���Ϊ��";
							UCHAR revBalance[4] = { 0x00 };
							for (int d = 0;d < 4;d++) {
								revBalance[d] = revdata[5 + d];
							}

							UCHAR balance[9] = {};
							Hex2Str(&revBalance[0], &balance[0], 4);
							for (int k = 0;k < 8;k++) {
								cout << balance[k];
							}
							cout << endl;
						}
						else if (revdata[4] == 0x03) {
							cout << "�����ݿ�" << block << "Ǯ����ֵ�ɹ��� ����ѯ���ʧ��" << endl;
						}
						else										//д���ݿ�ʧ��
						{
							cout << "��,д���ݿ�ʧ��,ʧ��ԭ�����£�" << endl;
							cout << "1. ���IC���Ƿ�����ڶ�д���ĸ�Ӧ����." << endl;
							cout << "2. IC����Ӧ�����������д����д���벻һ��." << endl;
							cout << "3. ��������ݿ�ֵ����IC����������ݿ���ֵ������S50����63�����ݿ�." << endl;
							cout << "4. ������ƿ鲻���Զ���д." << endl;
						}
					}
				}
				//�ۿ�
				if (revdata[0] == 0x01 && revdata[2] == 0xA7 && revdata[4] == 0x01) {
					cout << "Ǯ���ۿ�ʧ�ܣ���������¿۳�" << endl;
					return;
				}
				//�۳��ɹ������������
				//�����ۿ�ɹ�������ѯ���ʧ�ܵ����
				else if ((revdata[0] == 0x01) && (revdata[1] == 0x0A || revdata[1] == 0x08) && (revdata[2] == 0xA7) && (revdata[3] == 0x20)) { //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
					bool status = CheckSumIn(revdata, revdata[1]);  //����У���
					if (status)
					{
						if (revdata[4] == 0x00)						//д���ݿ�ɹ�
						{
							cout << "�����ݿ�" << block << "Ǯ���ۿ�ɹ��� ��ǰ���Ϊ��";
							UCHAR revBalance[4] = { 0x00 };
							for (int d = 0;d < 4;d++) {
								revBalance[d] = revdata[5 + d];
							}

							UCHAR balance[9] = {};
							Hex2Str(&revBalance[0], &balance[0], 4);
							for (int k = 0;k < 8;k++) {
								cout << balance[k];
							}
							cout << endl;
						}
						else if (revdata[4] == 0x03) {
							cout << "�����ݿ�" << block << "Ǯ���ۿ�ɹ��� ����ѯ���ʧ��" << endl;
						}
						else										//д���ݿ�ʧ��
						{
							cout << "��,д���ݿ�ʧ��,ʧ��ԭ�����£�" << endl;
							cout << "1. ���IC���Ƿ�����ڶ�д���ĸ�Ӧ����." << endl;
							cout << "2. IC����Ӧ�����������д����д���벻һ��." << endl;
							cout << "3. ��������ݿ�ֵ����IC����������ݿ���ֵ������S50����63�����ݿ�." << endl;
							cout << "4. ������ƿ鲻���Զ���д." << endl;
						}
					}
				}
			}
			else
			{
				cout << "��д����ʱ����������������������Ƿ�������" << endl;
				while (len > 0) //����������������ݣ������������������
				{
					mySerialPort->ReadChar(inbyte);
				}
			}
			break;
			//��ѯ���
		case 4:
		case 7:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//����У����
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //ͨ�����ڷ��Ͷ����ݿ�ָ�����д��
			Sleep(200);							 // ��ʱ200����ȴ���д���������ݣ���ʱ̫С�����޷��������������ݰ�
			len = mySerialPort->GetBytesInCOM(); //��ȡ���ڻ��������ֽ���
			if (len >= 8)
			{
				readbytes = 0;
				do								// ��ȡ���ڻ���������
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//���ܷ�������
						readbytes++;
					}
				} while (--len);

				//�жϷ��ص����ݰ���ʽ
				//��ѯ
				if (revdata[0] == 0x01 && revdata[2] == 0xA9 && revdata[4] == 0x01) {
					cout << "����ѯʧ�ܣ���������²�ѯ" << endl;
					return;
				}
				//��ѯ�ɹ������������
				else if ((revdata[0] == 0x01) && (revdata[1] == 0x0A) && (revdata[2] == 0xA9) && (revdata[3] == 0x20)) { //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
					bool status = CheckSumIn(revdata, revdata[1]);  //����У���
					if (status)
					{
						if (revdata[4] == 0x00)						//д���ݿ�ɹ�
						{
							cout << "�����ݿ�" << block << "��ѯ���ɹ��� ��ǰ���Ϊ��";
							UCHAR revBalance[4] = { 0x00 };
							for (int d = 0;d < 4;d++) {
								revBalance[d] = revdata[5 + d];
							}

							UCHAR balance[9] = {};
							Hex2Str(&revBalance[0], &balance[0], 4);
							for (int k = 0;k < 8;k++) {
								cout << balance[k];
							}
							cout << endl;
						}
						else										//д���ݿ�ʧ��
						{
							cout << "��,д���ݿ�ʧ��,ʧ��ԭ�����£�" << endl;
							cout << "1. ���IC���Ƿ�����ڶ�д���ĸ�Ӧ����." << endl;
							cout << "2. IC����Ӧ�����������д����д���벻һ��." << endl;
							cout << "3. ��������ݿ�ֵ����IC����������ݿ���ֵ������S50����63�����ݿ�." << endl;
							cout << "4. ������ƿ鲻���Զ���д." << endl;
						}
					}
				}
				//���÷�����״̬
				else if (revdata[0] == 0x03 && revdata[2] == 0xC2 && revdata[4] == 0x01) {
					cout << "���÷�����״̬ʧ�ܣ������ԣ�" << endl;
					return;
				}
				else if (revdata[0] == 0x03 && revdata[2] == 0xC2 && revdata[4] == 0x00) {
					if (CmdReadId[4] == 0x01) {
						cout << "�򿪷������ɹ�!" << endl;
					}
					else {
						cout << "�رշ������ɹ�!" << endl;
					}
				}
			}
			else
			{
				if (cin.fail())
				{
					cin.clear();
					cin.sync();
					cout << "******����Ŀ����������������0-64******" << endl << endl;
				}
			}
			break;
		case 8:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//����У����
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //ͨ�����ڷ��Ͷ����ݿ�ָ�����д��
			Sleep(200);							 // ��ʱ200����ȴ���д���������ݣ���ʱ̫С�����޷��������������ݰ�
			len = mySerialPort->GetBytesInCOM(); //��ȡ���ڻ��������ֽ���
			if (len >= 8)
			{
				readbytes = 0;
				do								// ��ȡ���ڻ���������
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//���ܷ�������
						readbytes++;
					}
				} while (--len);
				//ǿ�Ʒ����ɹ�
				if ((revdata[0] == 0x04) && (revdata[1] == 0x08) && (revdata[2] == 0xD1) && (revdata[3] == 0x20)) { //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
					bool status = CheckSumIn(revdata, revdata[1]);  //����У���
					if (status)
					{
						cout << "ǿ�ƿ����������ɹ���" << endl;
					}
				}
				//���÷�����״̬
				else{
					cout << "ǿ�ƹرշ������ɹ���" << endl;
					return;
				}
			}
			else {
				cout << "�Է�������ǿ�Ʋ���ʧ�ܣ�������" << endl;
			}
			break;
		case 9:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//����У����
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //ͨ�����ڷ��Ͷ����ݿ�ָ�����д��
			while (true) {
				Sleep(500);							 // ��ʱ200����ȴ���д���������ݣ���ʱ̫С�����޷��������������ݰ�
				len = mySerialPort->GetBytesInCOM(); //��ȡ���ڻ��������ֽ���
				if (len >= 8)
				{
					readbytes = 0;
					do								// ��ȡ���ڻ���������
					{
						inbyte = 0;
						if (mySerialPort->ReadChar(inbyte) == true)
						{
							revdata[readbytes] = inbyte;	//���ܷ�������
							readbytes++;
						}
					} while (--len);
					//�Զ�����

					if ((revdata[0] == 0x04) && (revdata[1] == 0x0C) && (revdata[2] == 0x02) && (revdata[3] == 0x20) && revdata[4] == 0x00) { //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
						bool status = CheckSumIn(revdata, revdata[1]);  //����У���
						
						if (status)
						{
							cout << "********->�Զ������ɹ���<-********" << endl;

							UCHAR balance[23] = {'0'};
							Hex2Str(&revdata[0], &balance[0], 11);
							
							cout << "===>��ǰ��ȡ�Ŀ����ͣ�";
							for (int k = 10;k < 14;k++) {
								cout << balance[k];
							}
							cout << ", �����ǣ�";
							for (int j = 14;j < 22;j++) {
								cout << balance[j];
							}
							cout << "<===\n\n������ɣ����Զ���ת��ϵͳ��ҳ�����Ժ󡤡���\n";
							cout << "==================================================" << endl;

							cout << endl;

							//����һ�������û�������ʾ��
							Sleep(3000);
						}
						return;
					}
					//�����Զ�������
					else {
						cout << "��ȡ��Ƭ����ʧ�ܣ������ԣ�" << endl;
						//return;
					}
				}
				else {
					//cout << "�Զ��������ݷ���ʧ�ܣ�������" << endl;
				}
			}
			break;
		case 10:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//����У����
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //ͨ�����ڷ��Ͷ����ݿ�ָ�����д��
			Sleep(200);							 // ��ʱ200����ȴ���д���������ݣ���ʱ̫С�����޷��������������ݰ�
			len = mySerialPort->GetBytesInCOM(); //��ȡ���ڻ��������ֽ���
			if (len >= 8)
			{
				readbytes = 0;
				do								// ��ȡ���ڻ���������
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//���ܷ�������
						readbytes++;
					}
				} while (--len);
				//�Զ�����
				if ((revdata[0] == 0x03) && (revdata[1] == 0x08) && (revdata[2] == 0xC1) && (revdata[3] == 0x20)) { //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
					bool status = CheckSumIn(revdata, revdata[1]);  //����У���
					if (status)
					{
						cout << "�Զ�����ģʽ���óɹ���" << endl;
					}
				}
				//�����Զ�������
				else {
					cout << "������ģʽ����ʧ�ܣ�" << endl;
					return;
				}
			}
			else {
				cout << "�Զ�����ģʽ�������ݷ���ʧ�ܣ�������" << endl;
			}
			break;
		case 11:

			break;
		case 12:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//����У����
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //ͨ�����ڷ��Ͷ����ݿ�ָ�����д��
			Sleep(200);							 // ��ʱ200����ȴ���д���������ݣ���ʱ̫С�����޷��������������ݰ�
			len = mySerialPort->GetBytesInCOM(); //��ȡ���ڻ��������ֽ���
			if (len >= 8)
			{
				readbytes = 0;
				do								// ��ȡ���ڻ���������
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//���ܷ�������
						readbytes++;
					}
				} while (--len);

				//Ӳ���汾������İ�������0x09����ָ����Ĳ�һ��
				if ((revdata[0] == 0x02) && (revdata[1] == 0x09) && (revdata[2] == 0xB6) && (revdata[3] == 0x20) && revdata[4] == 0x00) { //�ж��Ƿ�Ϊд���ݷ��ص����ݰ�
					bool status = CheckSumIn(revdata, revdata[1]);  //����У���

					if (status)
					{
						cout << "********->��ѯ��д��Ӳ���汾�ɹ���<-********" << endl;

						UCHAR balance[18] = { '0' };
						Hex2Str(&revdata[0], &balance[0], 8);

						cout << "===>��ǰ��д��Ӳ���汾��";
						cout <<"version" << balance[10] << '.' << balance[11] << endl;
						cout << endl << "���ڷ�����ҳ�����Ժ󡤡���" << endl;;
						cout << "==================================================" << endl;

						cout << endl;

						//����һ�������û�������ʾ��
						Sleep(2000);
					}
					return;
				}
				//�����Զ�������
				else {
					cout << "��ȡӲ���汾ʧ�ܣ������ԣ�" << endl;
					//return;
				}
			}
			else {
				cout << "��ȡӲ���汾ʧ�ܣ������ԣ�" << endl;
			}
			break;
		}
	}
}


/// <summary>
/// �Զ���ȡ��Ƭ�������װ
/// </summary>
/// <param name="mySerialPort"></param>
void autoReadYourCard(SerialPort* mySerialPort) {
	cout << "�뽫IC���Ŷ�д����Ӧ����" << endl;
	cout << "------------------------------------------------" << endl;
	//����8λ���ȵ������ʽ
	CmdReadId[0] = 0x04;
	CmdReadId[1] = 0x08;

	//�Զ�����
	CmdReadId[2] = 0xD0;

	CmdReadId[4] = 0x00;

	//����λ����00
	CmdReadId[5] = 0x00;
	CmdReadId[6] = 0x00;


	/// <summary>
	/// �������ݵ���λ��
	/// </summary>
	sendCommand(0x08, 9, mySerialPort);
}


/// <summary>
/// ��Ŀ��ʼ���Լ��û��ӿ�
/// </summary>
void init() {
	CHAR status;
	UCHAR inbyte;
	UCHAR indata[100];
	INT block;
	UINT i;
	//������״̬
	UINT buzzer = 0;

	//��ֵ���ۿ�Ľ�����̨����
	UCHAR money[8] = {'0'};

	/// <summary>
	/// ���ڶ�����ΪҪ���Σ��������������new��������һ��ָ�����
	/// </summary>
	/// <param name="argc"></param>
	/// <param name="argv"></param>
	/// <returns></returns>
	SerialPort* mySerialPort = new SerialPort();

	//��ʼ��ʧ��
	if (!mySerialPort->InitPort(PORT)) //��ʼ��COM������COM
	{
		cout << "==================================================" << endl;
		cout << "��ʼ��COM" << PORT << "ʧ�ܣ������д���˿��Ƿ�ΪCOM" << PORT << "�������Ƿ����������ռ�ã�" << endl;
		cout << "��������󣬻س��˳�����" << endl;
		cout << "==================================================" << endl;
		cin >> inbyte;
	}
	//��ʼ���ɹ�
	else
	{
		cout << "**********->��ʼ��COM" << PORT << "�ɹ���<-**********" << endl;
		cout << "\n==================================================" << endl;
		cout << "---����ϵͳ����״̬����ȡ��Ƭ��Ϣ����ܽ���ϵͳ---" << endl;
		cout << "==================================================\n" << endl;

		//�Զ���ȡ��Ƭ����ȡ��Ƭ����֮��Ż����ϵͳ
		autoReadYourCard(mySerialPort);

		cout << "==================================================" << endl;
		cout << "��ӭʹ�ö�дIC��ϵͳ������������س���" << endl;
		cout << "\t0. ������Ϣ" << endl;
		cout << "\t1. д���ݿ�" << endl;
		cout << "\t2. �����ݿ�" << endl;
		cout << "\t3. ����Ǯ��" << endl;
		cout << "\t4. Ǯ�����" << endl;
		cout << "\t5. ����ֵ" << endl;
		cout << "\t6. ʹ�ô��" << endl;
		cout << "\t7. ��������" << endl;
		cout << "\t8. ǿ�Ʒ���" << endl;
		cout << "\t9. �Զ����" << endl;
		cout << "\ta. ģʽ����" << endl;
		cout << "\tb. ����״̬" << endl;
		cout << "\tc. Ӳ���汾" << endl;
		cout << "\tq. �˳�ϵͳ" << endl;
		cout << "==================================================" << endl;

		//����ѭ������״̬
		while (true)
		{
			cout << "��������Ҫ���еĲ���������룺 ";
			cin >> inbyte;
			block = -1;
			status = -1;

			//��ʼ����ֵ�ۿ��Ϊ0
			for (int q = 0;q < 8;q++) {
				money[q] = '0';
			}


			cout << "==================================================" << endl;
			switch (inbyte)
			{
			case '0':
				//����8λ���ȵ������ʽ
				CmdReadId[0] = 0x01;
				CmdReadId[1] = 0x08;
				CmdReadId[2] = 0xA1;	//����������
				CmdReadId[4] = 0x00;
				CmdReadId[5] = 0x01;

				//����λ����00
				CmdReadId[6] = 0x00;
				//�����źͿ�����
				readIC(mySerialPort);
				break;
			case '1':
				cout << "�뽫IC���Ŷ�д����Ӧ���ڣ�����Ҫд�����ݵĿ�ţ��磺1�����س���" << endl;
				cout << "��������Ҫд�����ݵĴ洢��ţ� ";
				cin >> block;
				if (block > 0)
				{
					cout << "����Ҫд���16�������ݣ�0-F�����س���" << endl;
					cout << "------------------------------------------------" << endl;
					cout << "��������Ҫд��洢������ݣ� ";
					for (i = 0; i < 32; i++)
					{
						indata[i] = '0';
					}
					cin >> indata;

					for (int count = 0;count < 32;count++) {
						if (int(indata[count]) == 0) {
							indata[count] = '0';
							break;
						}
					}

					Cmd[1] = 0x17;
					Cmd[2] = 0xA4;							//A4��д���ݿ�����

					//�޸�Ŀ����
					Cmd[4] = (UCHAR)block;
					//�������ַ�תΪ�ֽ��룬������������
					HexStrToByte(&indata[0], &Cmd[6], 32);  //��������ַ�ת��16�����ֽ��������������飨�����
					/// <summary>
					/// �������ݵ���λ��
					/// </summary>
					sendCommand(block, 1, mySerialPort);
				}
				break;
			case '2':
				cout << "�뽫IC���Ŷ�д����Ӧ���ڣ�����Ҫ���Ŀ�ţ��磺1�����س��Կ�ʼ������" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "��������Ҫ��ѯ���ݵĴ洢��ţ� ";
				cin >> block;
				if (block > 0)
				{
					Cmd[1] = 0x08;
					Cmd[2] = 0xA3;				//�����ݿ�����
					Cmd[4] = (UCHAR)block;
					/// <summary>
					/// �������ݵ���λ��
					/// </summary>
					sendCommand(block, 2, mySerialPort);
				}
				break;
			case '3':
				cout << "�뽫IC���Ŷ�д����Ӧ���ڣ�����Ҫ����Ǯ���Ŀ�ţ��磺1�����س��Կ�ʼ��ʼ����" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "��������Ҫ����Ǯ���Ĵ洢��ţ� ";
				cin >> block;
				if (block > 0)
				{
					wallet_OP_Cmd[1] = 0x0B;
					wallet_OP_Cmd[2] = 0xA6;				//�����ݿ�����
					wallet_OP_Cmd[5] = 0x00;
					wallet_OP_Cmd[4] = (UCHAR)block;

					//��ʼ�����Ϊ0
					for (INT k = 6;k < 10;k++) {
						wallet_OP_Cmd[k] = 0x00;
					}

					/// <summary>
					/// �������ݵ���λ��
					/// </summary>
					sendCommand(block, 3, mySerialPort);
				}
				break;
			case '4':
				cout << "�뽫IC���Ŷ�д����Ӧ����" << endl;
				cout << "------------------------------------------------" << endl;
				
				CmdReadId[0] = 0x01;
				CmdReadId[1] = 0x08;
				CmdReadId[2] = 0xA9;	//��ѯ����
				CmdReadId[5] = 0x00;
				CmdReadId[4] = 0x08;	//�Ƚ����д��Ϊ0x08
				CmdReadId[6] = 0x00;

				/// <summary>
				/// �������ݵ���λ��
				/// </summary>
				sendCommand(0x08, 4, mySerialPort);
				break;
			case '5':
				cout << "�뽫IC���Ŷ�д����Ӧ���ڣ�����Ҫ��ֵ����Ŀ���س���" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "��������Ҫ��ֵ�Ľ� ";
				cin >> money;

				if (money[0] != '-')
				{
					wallet_OP_Cmd[1] = 0x0B;
					wallet_OP_Cmd[2] = 0xA8;	//��ֵ����
					wallet_OP_Cmd[5] = 0x00;
					wallet_OP_Cmd[4] = 0x08;	//�Ƚ����д��Ϊ0x08

					//�������뽫���ַ�����Ϊ'0'
					//�������bug�����ǰ�����Ľ��ָ����
					int dd = 0;
					for (dd = 0;dd < 8;dd++) {
						if (int(money[dd]) == 0) {
							money[dd] = '0';
							//����ASCII����Ϊ0��ֻ�����һ��
							break;
						}
					}

					//����IC���洢Ϊ��-�ߣ����԰��Լ���ʮ��������
					//	��תΪ��λ����λ
					//�Ƚ���Ч���ֺ��ƣ���ǰ�ò�0
					for (int pre = dd - 1, tail = 7;pre >= 0; pre--, tail--) {
						money[tail] = money[pre];
					}
					for (int zero_ind = 0;zero_ind < 8-dd;zero_ind++) {
						money[zero_ind] = '0';
					}

					//���ܳ�ֵ��Ŀ
					HexStrToByte(&money[0], &wallet_OP_Cmd[6], 8);

					//money�˴ε�ʹ���Ѿ��������ָ�
					for (int mLen = 0;mLen < 8;mLen++) {
						money[mLen] = '0';
					}
					/// <summary>
					/// �������ݵ���λ��
					/// </summary>
					sendCommand(0x08, 5, mySerialPort);
				}
				break;
			case '6':
				cout << "�뽫IC���Ŷ�д����Ӧ���ڣ�����Ҫʹ�õ���Ŀ���س���" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "��������Ҫ�۳��Ľ� ";
				cin >> money;

				if (money[0] != '-')
				{
					wallet_OP_Cmd[1] = 0x0B;
					wallet_OP_Cmd[2] = 0xA7;	//�ۿ�����
					wallet_OP_Cmd[5] = 0x00;
					wallet_OP_Cmd[4] = 0x08;	//�Ƚ����д��Ϊ0x08

					//�������뽫���ַ�����Ϊ'0'
					//�������bug�����ǰ�����Ľ��ָ����
					int dd = 0;
					for (dd = 0;dd < 8;dd++) {
						if (int(money[dd]) == 0) {
							money[dd] = '0';
							//����ASCII����Ϊ0��ֻ�����һ��
							break;
						}
					}

					//����IC���洢Ϊ��-�ߣ����԰��Լ���ʮ��������
					//	��תΪ��λ����λ
					//�Ƚ���Ч���ֺ��ƣ���ǰ�ò�0
					for (int pre = dd - 1, tail = 7;pre >= 0; pre--, tail--) {
						money[tail] = money[pre];
					}
					for (int zero_ind = 0;zero_ind < 8-dd;zero_ind++) {
						money[zero_ind] = '0';
					}

					//���ܿ۳���Ŀ
					HexStrToByte(&money[0], &wallet_OP_Cmd[6], 8);

					//money�˴ε�ʹ���Ѿ��������ָ�
					for (int mLen = 0;mLen < 8;mLen++) {
						money[mLen] = '0';
					}

					/// <summary>
					/// �������ݵ���λ��
					/// </summary>
					sendCommand(0x08, 6, mySerialPort);
				}
				break;
			case '7':
				cout << "�뽫IC���Ŷ�д����Ӧ����,�������õķ�����״̬" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "�����������״̬���루0-�رգ�1-�򿪣���";
				cin >> buzzer;
				if (buzzer != 0 && buzzer != 1) {
					cout << "�������������Ŷ��������0/1��" << endl;
					break;
				}
				//����8λ���ȵ������ʽ
				CmdReadId[0] = 0x03;
				CmdReadId[1] = 0x08;
				CmdReadId[2] = 0xC2;	//����������

				if (buzzer == 1) {
					CmdReadId[4] = 0x01;
				}
				else {
					CmdReadId[4] = 0x00;
				}
				
				//����λ����00
				CmdReadId[5] = 0x00;
				CmdReadId[6] = 0x00;

				/// <summary>
				/// �������ݵ���λ��
				/// </summary>
				sendCommand(0x08, 7, mySerialPort);
				break;
			case '8':
				cout << "�뽫IC���Ŷ�д����Ӧ����,�������õķ�����״̬" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "������ǿ�Ʒ�����״̬���루0-�رգ�1-�򿪣���";
				cin >> buzzer;
				if (buzzer != 0 && buzzer != 1) {
					cout << "�������������Ŷ��������0/1��" << endl;
					break;
				}
				//����8λ���ȵ������ʽ
				CmdReadId[0] = 0x04;
				CmdReadId[1] = 0x08;

				CmdReadId[4] = 0x00;

				//����λ����00
				CmdReadId[5] = 0x00;
				CmdReadId[6] = 0x00;

				//ǿ�Ʒ�����״̬
				if (buzzer == 1) {
					CmdReadId[2] = 0xD1;	//����������
				}
				else {
					CmdReadId[2] = 0xD2;	//����������
				}

				/// <summary>
				/// �������ݵ���λ��
				/// </summary>
				sendCommand(0x08, 8, mySerialPort);
				break;
			case '9':
				autoReadYourCard(mySerialPort);
				break;
			case 'a':
				cout << "�뽫IC���Ŷ�д����Ӧ����" << endl;
				cout << "------------------------------------------------" << endl;
				//����8λ���ȵ������ʽ
				CmdReadId[0] = 0x03;
				CmdReadId[1] = 0x08;

				CmdReadId[4] = 0x02;

				//����λ����00
				CmdReadId[5] = 0x08;
				CmdReadId[6] = 0x00;//������������

				//�Զ�����
				CmdReadId[2] = 0xC1;

				/// <summary>
				/// �������ݵ���λ��
				/// </summary>
				sendCommand(0x08, 10, mySerialPort);
				break;
			case 'b':
				cout << "������״̬��ѯ���ܴ�����" << endl;
				break;
			case 'c':
				cout << "��ѯ��ǰʹ�õĶ�д���汾��" << endl;
				cout << "------------------------------------------------" << endl;
				//����8λ���ȵ������ʽ
				CmdReadId[0] = 0x02;
				CmdReadId[1] = 0x08;

				CmdReadId[4] = 0x00;

				//����λ����00
				CmdReadId[5] = 0x00;
				CmdReadId[6] = 0x00;//������������

				//��ѯ�汾
				CmdReadId[2] = 0xB6;

				/// <summary>
				/// �������ݵ���λ��
				/// </summary>
				sendCommand(0x08, 12, mySerialPort);
				break;
			case 'q':
				cout << "================���˳�ϵͳ���ڴ��´μ��棡================" << endl;;
				system("pause");
				exit(0);
				break;
			default:
				cout << "******δ֪�����������ȷ�Ĳ����������******" << endl;
			}
			
			/*�ٴ������ʾ��Ϣ����ʾ�û�����*/
			cout << "\n==================================================" << endl;
			cout << "��ѡ���Ӧ�Ĺ��ܴ��룬���Իس�������" << endl;
			cout << "\t0. ��ȡ����Ϣ" << endl;
			cout << "\t1. д���ݿ�" << endl;
			cout << "\t2. �����ݿ�" << endl;
			cout << "\t3. ����Ǯ��" << endl;
			cout << "\t4. Ǯ�����" << endl;
			cout << "\t5. ����ֵ" << endl;
			cout << "\t6. ʹ�ô��" << endl;
			cout << "\t7. ����״̬" << endl;
			cout << "\t8. ǿ�Ʒ���" << endl;
			cout << "\t9. �Զ����" << endl;
			cout << "\ta. ģʽ����" << endl;
			cout << "\tb. ����״̬" << endl;
			cout << "\tc. Ӳ���汾" << endl;
			cout << "\tq. �˳�ϵͳ" << endl;
			cout << "==================================================" << endl;
		}
	}
}


/// <summary>
/// ������-��ʼ���Լ������������
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int main(int argc, _TCHAR* argv[])
{
	init();			// ͨ��������ʼ�����������û��ӿ�
}

