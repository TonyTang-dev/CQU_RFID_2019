// ConsoleApplication1.cpp : 定义控制台应用程序的入口点。

#include "stdafx.h"  
#include "SerialPort.h"
#include <process.h>  
#include <iostream>

using namespace std;

/*
	全局变量声明及宏定义
*/
//串口端口号
#define PORT 2

/// <summary>
/// 读卡号的前配置，上位机包格式
/// </summary>
UCHAR  CmdReadId[8] = { 0x01, 0x08, 0xA1, 0x20, 0x00, 0x01, 0x00, 0x76 };

/// <summary>
/// 读写卡命令
/// </summary>
UCHAR  Cmd[23] = { 0x01, 0x17, 0xA4, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/// <summary>
/// 创建钱包、充值、扣款操作命令格式
/// </summary>
UCHAR wallet_OP_Cmd[11] = { 0x01, 0x0B, 0xA6, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/// <summary>
/// 查询钱包余额命令格式
/// </summary>
UCHAR queryWallet_Cmd[8] = { 0x01, 0x08, 0xA9, 0x20, 0x00, 0x00, 0x00, 0x00 };


/// <summary>
/// 校验码
/// </summary>
/// <param name="buf">buf：缓冲区数据</param>
/// <param name="len">len：数据长度</param>
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
/// 校验码
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
/// 字节流转换为十六进制字符串的另一种实现方式 
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
/// 十六进制字符串转换为字节流  
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
/// 读卡号函数封装
/// </summary>
/// <param name="argc">串口对象：*mySerialPort</param>
/// <param name="argv"></param>
/// <returns>none</returns>

void readIC(SerialPort *mySerialPort) {
	UCHAR inbyte;
	UCHAR revdata[32];
	UINT len = 0;
	UINT readbytes;
	CheckSumOut(CmdReadId, CmdReadId[1]);
	mySerialPort->WriteData(CmdReadId, CmdReadId[1]);
	Sleep(200);								// 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
	len = mySerialPort->GetBytesInCOM();	//获取串口缓冲区中字节数
	if (len >= 8)							// 读卡号读卡器返回的数据包长度：失败为8字节，成功为12字节
	{
		readbytes = 0;
		do									// 获取串口缓冲区数据
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
					UCHAR cardtype[5];						//保存卡类型
					UCHAR id[9];							//保存卡号
					Hex2Str(&revdata[5], &cardtype[0], 2);	// 数组revdata[5]开始2字节为卡类型
					Hex2Str(&revdata[7], &id[0], 4);		// 数组revdata[7]开始4字节为卡号
					cout << "当前IC卡类型:" << cardtype << "，卡号：" << id << endl;
				}
				else
				{
					cout << "未检测到卡，将IC放置到读写器感情区才可读卡！" << endl;
				}
			}
		}
	}
	else
	{
		cout << "读卡超时，请检查读卡器的连接是否正常！" << endl;
		while (len > 0)
		{
			mySerialPort->ReadChar(inbyte);
		}
	}

	return;
}


/// <summary>
/// 上位机发送命令给下位机
/// </summary>
/// <param name="block">操作的块号</param>
/// <param name="cmd">操作的命令类型
///		写块号：		1
///		读块号：		2
///		初始化钱包：	3
///		查询余额：	4
///		钱包充值：	5
///		使用钱包：	6
///		设置蜂鸣器状态：7
/// </param>
/// /// <param name="mySerialPort">串口对象实例</param>
void sendCommand(INT block, INT cmd, SerialPort *mySerialPort) {
	//数据长度
	UINT len = 0;
	//接受的数据
	UCHAR revdata[100];
	//读取的字节数
	UINT readbytes;
	//缓冲区字符
	UCHAR inbyte;
	//递增变量
	UINT i = 0;

	if (block > 0)
	{
		switch (cmd) {
			//写数据块
			//读数据块
		case 1:
		case 2:
			CheckSumOut(Cmd, Cmd[1]);				//计算校验码
			mySerialPort->WriteData(Cmd, Cmd[1]);  //通过串口发送读数据块指令给读写器
			Sleep(200);							 // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
			len = mySerialPort->GetBytesInCOM(); //获取串口缓冲区中字节数
			if (len >= 8)
			{
				readbytes = 0;
				do								// 获取串口缓冲区数据
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//接受返回数据
						readbytes++;
					}
				} while (--len);

				//判断返回的数据包格式
				if ((revdata[0] == 0x01) && (revdata[1] == 8) && (revdata[1] == readbytes) && (revdata[2] == 0x0A4) && (revdata[3] = 0x20)) //判断是否为写数据返回的数据包
				{
					bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
					if (status)
					{
						if (revdata[4] == 0x00)						//写数据块成功
						{
							cout << "写数据到数据块" << block << "成功！" << endl << endl;
						}
						else										//写数据块失败
						{
							cout << "读,写数据块失败,失败原因如下：" << endl;
							cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
							cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
							cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块." << endl;
							cout << "4. 密码控制块不可以读或写." << endl;
						}
					}
				}
				if ((revdata[0] == 0x01) && ((revdata[1] == 8) || (revdata[1] == 22)) && (revdata[1] == readbytes) && (revdata[2] == 0xA3) && (revdata[3] = 0x20))//判断是否为读数据块返回的数据包
				{
					bool status = CheckSumIn(revdata, revdata[1]);	//计算校验和
					if (status)
					{
						if (revdata[4] == 0x00)						//读数据块成功
						{
							UCHAR blockdata[16];
							UCHAR temp[33];
							for (i = 0; i < 16; i++)
							{
								blockdata[i] = revdata[5 + i];		//复制数据到数组
							}
							Hex2Str(&blockdata[0], &temp[0], 16);	// 数据块数据转换为字符
							cout << "读数据块成功，数据块" << block << "数据为：" << temp << endl << endl;
						}
						else										//读数据块失败
						{
							cout << "读,写数据块失败,失败原因如下：" << endl;
							cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
							cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
							cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块." << endl;
							cout << "4. 密码控制块不可以读或写." << endl;
						}
					}
				}
			}
			else
			{
				cout << "读写器超时……，请检查读卡器的连接是否正常！" << endl;
				while (len > 0) //如果缓冲区中有数据，将缓冲区中数据清空
				{
					mySerialPort->ReadChar(inbyte);
				}
			}
			break;

		case 3://创建钱包
		case 5://钱包充值
		case 6://使用钱包
			CheckSumOut(wallet_OP_Cmd, wallet_OP_Cmd[1]);				//计算校验码
			mySerialPort->WriteData(wallet_OP_Cmd, wallet_OP_Cmd[1]);  //通过串口发送读数据块指令给读写器
			Sleep(200);							 // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
			len = mySerialPort->GetBytesInCOM(); //获取串口缓冲区中字节数
			if (len >= 8)
			{
				readbytes = 0;
				do								// 获取串口缓冲区数据
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//接受返回数据
						readbytes++;
					}
				} while (--len);

				//判断返回的数据包格式
				//初始化失败
				if (revdata[0] == 0x01 && revdata[2] == 0xA6 && revdata[4] == 0x01) {
					cout << "初始化钱包失败，请检查后重新创建" << endl;
					return;
				}
				//初始化成功，并返回余额
				//包括初始化成功，但查询余额失败的情况
				else if ((revdata[0] == 0x01) && (revdata[1] == 0x0A || revdata[1] == 0x08) && (revdata[2] == 0xA6) && (revdata[3] == 0x20)) { //判断是否为写数据返回的数据包
					bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
					if (status)
					{
						if (revdata[4] == 0x00)						//写数据块成功
						{
							cout << "在数据块" << block << "创建钱包成功！ 当前余额为：";

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
							cout << "在数据块" << block << "创建钱包成功！ 但查询余额失败" << endl;
						}
						else										//写数据块失败
						{
							cout << "读,写数据块失败,失败原因如下：" << endl;
							cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
							cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
							cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块." << endl;
							cout << "4. 密码控制块不可以读或写." << endl;
						}
					}
					return;
				}

				//充值
				if (revdata[0] == 0x01 && revdata[2] == 0xA8 && revdata[4] == 0x01) {
					cout << "钱包充值失败，请检查后重新充值" << endl;
					return;
				}
				//充值成功，并返回余额
				//包括充值成功，但查询余额失败的情况
				else if ((revdata[0] == 0x01) && (revdata[1] == 0x0A || revdata[1] == 0x08) && (revdata[2] == 0xA8) && (revdata[3] == 0x20)) { //判断是否为写数据返回的数据包
					bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
					if (status)
					{
						if (revdata[4] == 0x00)						//写数据块成功
						{
							cout << "在数据块" << block << "钱包充值成功！ 当前余额为：";
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
							cout << "在数据块" << block << "钱包充值成功！ 但查询余额失败" << endl;
						}
						else										//写数据块失败
						{
							cout << "读,写数据块失败,失败原因如下：" << endl;
							cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
							cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
							cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块." << endl;
							cout << "4. 密码控制块不可以读或写." << endl;
						}
					}
				}
				//扣款
				if (revdata[0] == 0x01 && revdata[2] == 0xA7 && revdata[4] == 0x01) {
					cout << "钱包扣款失败，请检查后重新扣除" << endl;
					return;
				}
				//扣除成功，并返回余额
				//包括扣款成功，但查询余额失败的情况
				else if ((revdata[0] == 0x01) && (revdata[1] == 0x0A || revdata[1] == 0x08) && (revdata[2] == 0xA7) && (revdata[3] == 0x20)) { //判断是否为写数据返回的数据包
					bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
					if (status)
					{
						if (revdata[4] == 0x00)						//写数据块成功
						{
							cout << "在数据块" << block << "钱包扣款成功！ 当前余额为：";
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
							cout << "在数据块" << block << "钱包扣款成功！ 但查询余额失败" << endl;
						}
						else										//写数据块失败
						{
							cout << "读,写数据块失败,失败原因如下：" << endl;
							cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
							cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
							cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块." << endl;
							cout << "4. 密码控制块不可以读或写." << endl;
						}
					}
				}
			}
			else
			{
				cout << "读写器超时……，请检查读卡器的连接是否正常！" << endl;
				while (len > 0) //如果缓冲区中有数据，将缓冲区中数据清空
				{
					mySerialPort->ReadChar(inbyte);
				}
			}
			break;
			//查询余额
		case 4:
		case 7:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//计算校验码
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //通过串口发送读数据块指令给读写器
			Sleep(200);							 // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
			len = mySerialPort->GetBytesInCOM(); //获取串口缓冲区中字节数
			if (len >= 8)
			{
				readbytes = 0;
				do								// 获取串口缓冲区数据
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//接受返回数据
						readbytes++;
					}
				} while (--len);

				//判断返回的数据包格式
				//查询
				if (revdata[0] == 0x01 && revdata[2] == 0xA9 && revdata[4] == 0x01) {
					cout << "余额查询失败，请检查后重新查询" << endl;
					return;
				}
				//查询成功，并返回余额
				else if ((revdata[0] == 0x01) && (revdata[1] == 0x0A) && (revdata[2] == 0xA9) && (revdata[3] == 0x20)) { //判断是否为写数据返回的数据包
					bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
					if (status)
					{
						if (revdata[4] == 0x00)						//写数据块成功
						{
							cout << "在数据块" << block << "查询余额成功！ 当前余额为：";
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
						else										//写数据块失败
						{
							cout << "读,写数据块失败,失败原因如下：" << endl;
							cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
							cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
							cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块." << endl;
							cout << "4. 密码控制块不可以读或写." << endl;
						}
					}
				}
				//设置蜂鸣器状态
				else if (revdata[0] == 0x03 && revdata[2] == 0xC2 && revdata[4] == 0x01) {
					cout << "设置蜂鸣器状态失败，请重试！" << endl;
					return;
				}
				else if (revdata[0] == 0x03 && revdata[2] == 0xC2 && revdata[4] == 0x00) {
					if (CmdReadId[4] == 0x01) {
						cout << "打开蜂鸣器成功!" << endl;
					}
					else {
						cout << "关闭蜂鸣器成功!" << endl;
					}
				}
			}
			else
			{
				if (cin.fail())
				{
					cin.clear();
					cin.sync();
					cout << "******输入的块号有误，请输入数字0-64******" << endl << endl;
				}
			}
			break;
		case 8:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//计算校验码
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //通过串口发送读数据块指令给读写器
			Sleep(200);							 // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
			len = mySerialPort->GetBytesInCOM(); //获取串口缓冲区中字节数
			if (len >= 8)
			{
				readbytes = 0;
				do								// 获取串口缓冲区数据
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//接受返回数据
						readbytes++;
					}
				} while (--len);
				//强制蜂鸣成功
				if ((revdata[0] == 0x04) && (revdata[1] == 0x08) && (revdata[2] == 0xD1) && (revdata[3] == 0x20)) { //判断是否为写数据返回的数据包
					bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
					if (status)
					{
						cout << "强制开启蜂鸣器成功！" << endl;
					}
				}
				//设置蜂鸣器状态
				else{
					cout << "强制关闭蜂鸣器成功！" << endl;
					return;
				}
			}
			else {
				cout << "对蜂鸣器的强制操作失败，请重试" << endl;
			}
			break;
		case 9:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//计算校验码
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //通过串口发送读数据块指令给读写器
			while (true) {
				Sleep(500);							 // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
				len = mySerialPort->GetBytesInCOM(); //获取串口缓冲区中字节数
				if (len >= 8)
				{
					readbytes = 0;
					do								// 获取串口缓冲区数据
					{
						inbyte = 0;
						if (mySerialPort->ReadChar(inbyte) == true)
						{
							revdata[readbytes] = inbyte;	//接受返回数据
							readbytes++;
						}
					} while (--len);
					//自动读卡

					if ((revdata[0] == 0x04) && (revdata[1] == 0x0C) && (revdata[2] == 0x02) && (revdata[3] == 0x20) && revdata[4] == 0x00) { //判断是否为写数据返回的数据包
						bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
						
						if (status)
						{
							cout << "********->自动读卡成功！<-********" << endl;

							UCHAR balance[23] = {'0'};
							Hex2Str(&revdata[0], &balance[0], 11);
							
							cout << "===>当前读取的卡类型：";
							for (int k = 10;k < 14;k++) {
								cout << balance[k];
							}
							cout << ", 卡号是：";
							for (int j = 14;j < 22;j++) {
								cout << balance[j];
							}
							cout << "<===\n\n读卡完成，将自动跳转到系统主页，请稍后···\n";
							cout << "==================================================" << endl;

							cout << endl;

							//休眠一秒以让用户看到提示语
							Sleep(3000);
						}
						return;
					}
					//不是自动读卡号
					else {
						cout << "读取卡片数据失败，请重试！" << endl;
						//return;
					}
				}
				else {
					//cout << "自动读卡数据返回失败，请重试" << endl;
				}
			}
			break;
		case 10:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//计算校验码
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //通过串口发送读数据块指令给读写器
			Sleep(200);							 // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
			len = mySerialPort->GetBytesInCOM(); //获取串口缓冲区中字节数
			if (len >= 8)
			{
				readbytes = 0;
				do								// 获取串口缓冲区数据
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//接受返回数据
						readbytes++;
					}
				} while (--len);
				//自动读卡
				if ((revdata[0] == 0x03) && (revdata[1] == 0x08) && (revdata[2] == 0xC1) && (revdata[3] == 0x20)) { //判断是否为写数据返回的数据包
					bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
					if (status)
					{
						cout << "自动读卡模式设置成功！" << endl;
					}
				}
				//不是自动读卡号
				else {
					cout << "动读卡模式设置失败！" << endl;
					return;
				}
			}
			else {
				cout << "自动读卡模式设置数据返回失败，请重试" << endl;
			}
			break;
		case 11:

			break;
		case 12:
			CheckSumOut(CmdReadId, CmdReadId[1]);				//计算校验码
			mySerialPort->WriteData(CmdReadId, CmdReadId[1]);  //通过串口发送读数据块指令给读写器
			Sleep(200);							 // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
			len = mySerialPort->GetBytesInCOM(); //获取串口缓冲区中字节数
			if (len >= 8)
			{
				readbytes = 0;
				do								// 获取串口缓冲区数据
				{
					inbyte = 0;
					if (mySerialPort->ReadChar(inbyte) == true)
					{
						revdata[readbytes] = inbyte;	//接受返回数据
						readbytes++;
					}
				} while (--len);

				//硬件版本，这里的包长度是0x09，与指导书的不一样
				if ((revdata[0] == 0x02) && (revdata[1] == 0x09) && (revdata[2] == 0xB6) && (revdata[3] == 0x20) && revdata[4] == 0x00) { //判断是否为写数据返回的数据包
					bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和

					if (status)
					{
						cout << "********->查询读写器硬件版本成功！<-********" << endl;

						UCHAR balance[18] = { '0' };
						Hex2Str(&revdata[0], &balance[0], 8);

						cout << "===>当前读写器硬件版本：";
						cout <<"version" << balance[10] << '.' << balance[11] << endl;
						cout << endl << "正在返回主页，请稍后···" << endl;;
						cout << "==================================================" << endl;

						cout << endl;

						//休眠一秒以让用户看到提示语
						Sleep(2000);
					}
					return;
				}
				//不是自动读卡号
				else {
					cout << "读取硬件版本失败，请重试！" << endl;
					//return;
				}
			}
			else {
				cout << "读取硬件版本失败，请重试！" << endl;
			}
			break;
		}
	}
}


/// <summary>
/// 自动读取卡片命令函数封装
/// </summary>
/// <param name="mySerialPort"></param>
void autoReadYourCard(SerialPort* mySerialPort) {
	cout << "请将IC卡放读写器感应区内" << endl;
	cout << "------------------------------------------------" << endl;
	//重用8位长度的命令格式
	CmdReadId[0] = 0x04;
	CmdReadId[1] = 0x08;

	//自动读卡
	CmdReadId[2] = 0xD0;

	CmdReadId[4] = 0x00;

	//数据位保持00
	CmdReadId[5] = 0x00;
	CmdReadId[6] = 0x00;


	/// <summary>
	/// 发送数据到下位机
	/// </summary>
	sendCommand(0x08, 9, mySerialPort);
}


/// <summary>
/// 项目初始化以及用户接口
/// </summary>
void init() {
	CHAR status;
	UCHAR inbyte;
	UCHAR indata[100];
	INT block;
	UINT i;
	//蜂鸣器状态
	UINT buzzer = 0;

	//充值、扣款的金额控制台输入
	UCHAR money[8] = {'0'};

	/// <summary>
	/// 串口对象，因为要传参，所以这里改用了new操作声明一个指针对象
	/// </summary>
	/// <param name="argc"></param>
	/// <param name="argv"></param>
	/// <returns></returns>
	SerialPort* mySerialPort = new SerialPort();

	//初始化失败
	if (!mySerialPort->InitPort(PORT)) //初始化COM，并打开COM
	{
		cout << "==================================================" << endl;
		cout << "初始化COM" << PORT << "失败，请检查读写器端口是否为COM" << PORT << "，或者是否被其它软件打开占用！" << endl;
		cout << "按任意键后，回车退出程序！" << endl;
		cout << "==================================================" << endl;
		cin >> inbyte;
	}
	//初始化成功
	else
	{
		cout << "**********->初始化COM" << PORT << "成功！<-**********" << endl;
		cout << "\n==================================================" << endl;
		cout << "---进入系统监听状态，获取卡片信息后才能进入系统---" << endl;
		cout << "==================================================\n" << endl;

		//自动读取卡片，获取卡片数据之后才会进入系统
		autoReadYourCard(mySerialPort);

		cout << "==================================================" << endl;
		cout << "欢迎使用读写IC卡系统，请输入命令并回车：" << endl;
		cout << "\t0. 读卡信息" << endl;
		cout << "\t1. 写数据块" << endl;
		cout << "\t2. 读数据块" << endl;
		cout << "\t3. 创建钱包" << endl;
		cout << "\t4. 钱包余额" << endl;
		cout << "\t5. 存款充值" << endl;
		cout << "\t6. 使用存款" << endl;
		cout << "\t7. 蜂鸣设置" << endl;
		cout << "\t8. 强制蜂鸣" << endl;
		cout << "\t9. 自动检测" << endl;
		cout << "\ta. 模式设置" << endl;
		cout << "\tb. 蜂鸣状态" << endl;
		cout << "\tc. 硬件版本" << endl;
		cout << "\tq. 退出系统" << endl;
		cout << "==================================================" << endl;

		//进入循环监听状态
		while (true)
		{
			cout << "请输入您要进行的操作命令代码： ";
			cin >> inbyte;
			block = -1;
			status = -1;

			//初始化充值扣款缓存为0
			for (int q = 0;q < 8;q++) {
				money[q] = '0';
			}


			cout << "==================================================" << endl;
			switch (inbyte)
			{
			case '0':
				//重用8位长度的命令格式
				CmdReadId[0] = 0x01;
				CmdReadId[1] = 0x08;
				CmdReadId[2] = 0xA1;	//蜂鸣器命令
				CmdReadId[4] = 0x00;
				CmdReadId[5] = 0x01;

				//数据位保持00
				CmdReadId[6] = 0x00;
				//读卡号和卡类型
				readIC(mySerialPort);
				break;
			case '1':
				cout << "请将IC卡放读写器感应区内，输入要写入数据的块号（如：1）并回车：" << endl;
				cout << "请输入您要写入数据的存储块号： ";
				cin >> block;
				if (block > 0)
				{
					cout << "输入要写入的16进制数据（0-F）并回车：" << endl;
					cout << "------------------------------------------------" << endl;
					cout << "请输入您要写入存储块的数据： ";
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
					Cmd[2] = 0xA4;							//A4是写数据块命令

					//修改目标块号
					Cmd[4] = (UCHAR)block;
					//将数据字符转为字节码，拷贝到数据区
					HexStrToByte(&indata[0], &Cmd[6], 32);  //将输入的字符转成16进制字节数并拷贝到数组（命令）中
					/// <summary>
					/// 发送数据到下位机
					/// </summary>
					sendCommand(block, 1, mySerialPort);
				}
				break;
			case '2':
				cout << "请将IC卡放读写器感应区内，输入要读的块号（如：1）并回车以开始读卡：" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "请输入您要查询数据的存储块号： ";
				cin >> block;
				if (block > 0)
				{
					Cmd[1] = 0x08;
					Cmd[2] = 0xA3;				//读数据块命令
					Cmd[4] = (UCHAR)block;
					/// <summary>
					/// 发送数据到下位机
					/// </summary>
					sendCommand(block, 2, mySerialPort);
				}
				break;
			case '3':
				cout << "请将IC卡放读写器感应区内，输入要创建钱包的块号（如：1）并回车以开始初始化：" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "请输入您要创建钱包的存储块号： ";
				cin >> block;
				if (block > 0)
				{
					wallet_OP_Cmd[1] = 0x0B;
					wallet_OP_Cmd[2] = 0xA6;				//读数据块命令
					wallet_OP_Cmd[5] = 0x00;
					wallet_OP_Cmd[4] = (UCHAR)block;

					//初始化余额为0
					for (INT k = 6;k < 10;k++) {
						wallet_OP_Cmd[k] = 0x00;
					}

					/// <summary>
					/// 发送数据到下位机
					/// </summary>
					sendCommand(block, 3, mySerialPort);
				}
				break;
			case '4':
				cout << "请将IC卡放读写器感应区内" << endl;
				cout << "------------------------------------------------" << endl;
				
				CmdReadId[0] = 0x01;
				CmdReadId[1] = 0x08;
				CmdReadId[2] = 0xA9;	//查询命令
				CmdReadId[5] = 0x00;
				CmdReadId[4] = 0x08;	//先将块号写死为0x08
				CmdReadId[6] = 0x00;

				/// <summary>
				/// 发送数据到下位机
				/// </summary>
				sendCommand(0x08, 4, mySerialPort);
				break;
			case '5':
				cout << "请将IC卡放读写器感应区内，输入要充值的数目并回车：" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "请输入您要充值的金额： ";
				cin >> money;

				if (money[0] != '-')
				{
					wallet_OP_Cmd[1] = 0x0B;
					wallet_OP_Cmd[2] = 0xA8;	//充值命令
					wallet_OP_Cmd[5] = 0x00;
					wallet_OP_Cmd[4] = 0x08;	//先将块号写死为0x08

					//遍历输入将空字符设置为'0'
					//利用这个bug，我们把输入的金额分割出来
					int dd = 0;
					for (dd = 0;dd < 8;dd++) {
						if (int(money[dd]) == 0) {
							money[dd] = '0';
							//由于ASCII码字为0的只会出现一个
							break;
						}
					}

					//由于IC卡存储为低-高，所以把自己的十进制输入
					//	反转为低位到高位
					//先将有效数字后移，再前置补0
					for (int pre = dd - 1, tail = 7;pre >= 0; pre--, tail--) {
						money[tail] = money[pre];
					}
					for (int zero_ind = 0;zero_ind < 8-dd;zero_ind++) {
						money[zero_ind] = '0';
					}

					//接受充值数目
					HexStrToByte(&money[0], &wallet_OP_Cmd[6], 8);

					//money此次的使命已经结束，恢复
					for (int mLen = 0;mLen < 8;mLen++) {
						money[mLen] = '0';
					}
					/// <summary>
					/// 发送数据到下位机
					/// </summary>
					sendCommand(0x08, 5, mySerialPort);
				}
				break;
			case '6':
				cout << "请将IC卡放读写器感应区内，输入要使用的数目并回车：" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "请输入您要扣除的金额： ";
				cin >> money;

				if (money[0] != '-')
				{
					wallet_OP_Cmd[1] = 0x0B;
					wallet_OP_Cmd[2] = 0xA7;	//扣款命令
					wallet_OP_Cmd[5] = 0x00;
					wallet_OP_Cmd[4] = 0x08;	//先将块号写死为0x08

					//遍历输入将空字符设置为'0'
					//利用这个bug，我们把输入的金额分割出来
					int dd = 0;
					for (dd = 0;dd < 8;dd++) {
						if (int(money[dd]) == 0) {
							money[dd] = '0';
							//由于ASCII码字为0的只会出现一个
							break;
						}
					}

					//由于IC卡存储为低-高，所以把自己的十进制输入
					//	反转为低位到高位
					//先将有效数字后移，再前置补0
					for (int pre = dd - 1, tail = 7;pre >= 0; pre--, tail--) {
						money[tail] = money[pre];
					}
					for (int zero_ind = 0;zero_ind < 8-dd;zero_ind++) {
						money[zero_ind] = '0';
					}

					//接受扣除数目
					HexStrToByte(&money[0], &wallet_OP_Cmd[6], 8);

					//money此次的使命已经结束，恢复
					for (int mLen = 0;mLen < 8;mLen++) {
						money[mLen] = '0';
					}

					/// <summary>
					/// 发送数据到下位机
					/// </summary>
					sendCommand(0x08, 6, mySerialPort);
				}
				break;
			case '7':
				cout << "请将IC卡放读写器感应区内,输入设置的蜂鸣器状态" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "请输入蜂鸣器状态代码（0-关闭；1-打开）：";
				cin >> buzzer;
				if (buzzer != 0 && buzzer != 1) {
					cout << "输入的命令有误哦，请输入0/1！" << endl;
					break;
				}
				//重用8位长度的命令格式
				CmdReadId[0] = 0x03;
				CmdReadId[1] = 0x08;
				CmdReadId[2] = 0xC2;	//蜂鸣器命令

				if (buzzer == 1) {
					CmdReadId[4] = 0x01;
				}
				else {
					CmdReadId[4] = 0x00;
				}
				
				//数据位保持00
				CmdReadId[5] = 0x00;
				CmdReadId[6] = 0x00;

				/// <summary>
				/// 发送数据到下位机
				/// </summary>
				sendCommand(0x08, 7, mySerialPort);
				break;
			case '8':
				cout << "请将IC卡放读写器感应区内,输入设置的蜂鸣器状态" << endl;
				cout << "------------------------------------------------" << endl;
				cout << "请输入强制蜂鸣器状态代码（0-关闭；1-打开）：";
				cin >> buzzer;
				if (buzzer != 0 && buzzer != 1) {
					cout << "输入的命令有误哦，请输入0/1！" << endl;
					break;
				}
				//重用8位长度的命令格式
				CmdReadId[0] = 0x04;
				CmdReadId[1] = 0x08;

				CmdReadId[4] = 0x00;

				//数据位保持00
				CmdReadId[5] = 0x00;
				CmdReadId[6] = 0x00;

				//强制蜂鸣器状态
				if (buzzer == 1) {
					CmdReadId[2] = 0xD1;	//蜂鸣器命令
				}
				else {
					CmdReadId[2] = 0xD2;	//蜂鸣器命令
				}

				/// <summary>
				/// 发送数据到下位机
				/// </summary>
				sendCommand(0x08, 8, mySerialPort);
				break;
			case '9':
				autoReadYourCard(mySerialPort);
				break;
			case 'a':
				cout << "请将IC卡放读写器感应区内" << endl;
				cout << "------------------------------------------------" << endl;
				//重用8位长度的命令格式
				CmdReadId[0] = 0x03;
				CmdReadId[1] = 0x08;

				CmdReadId[4] = 0x02;

				//数据位保持00
				CmdReadId[5] = 0x08;
				CmdReadId[6] = 0x00;//主动发送数据

				//自动读卡
				CmdReadId[2] = 0xC1;

				/// <summary>
				/// 发送数据到下位机
				/// </summary>
				sendCommand(0x08, 10, mySerialPort);
				break;
			case 'b':
				cout << "蜂鸣器状态查询功能待完善" << endl;
				break;
			case 'c':
				cout << "查询当前使用的读写器版本号" << endl;
				cout << "------------------------------------------------" << endl;
				//重用8位长度的命令格式
				CmdReadId[0] = 0x02;
				CmdReadId[1] = 0x08;

				CmdReadId[4] = 0x00;

				//数据位保持00
				CmdReadId[5] = 0x00;
				CmdReadId[6] = 0x00;//主动发送数据

				//查询版本
				CmdReadId[2] = 0xB6;

				/// <summary>
				/// 发送数据到下位机
				/// </summary>
				sendCommand(0x08, 12, mySerialPort);
				break;
			case 'q':
				cout << "================已退出系统，期待下次见面！================" << endl;;
				system("pause");
				exit(0);
				break;
			default:
				cout << "******未知命令，请输入正确的操作命令代码******" << endl;
			}
			
			/*再次输出提示信息以提示用户输入*/
			cout << "\n==================================================" << endl;
			cout << "请选择对应的功能代码，并以回车结束：" << endl;
			cout << "\t0. 读取卡信息" << endl;
			cout << "\t1. 写数据块" << endl;
			cout << "\t2. 读数据块" << endl;
			cout << "\t3. 创建钱包" << endl;
			cout << "\t4. 钱包余额" << endl;
			cout << "\t5. 存款充值" << endl;
			cout << "\t6. 使用存款" << endl;
			cout << "\t7. 蜂鸣状态" << endl;
			cout << "\t8. 强制蜂鸣" << endl;
			cout << "\t9. 自动检测" << endl;
			cout << "\ta. 模式设置" << endl;
			cout << "\tb. 蜂鸣状态" << endl;
			cout << "\tc. 硬件版本" << endl;
			cout << "\tq. 退出系统" << endl;
			cout << "==================================================" << endl;
		}
	}
}


/// <summary>
/// 主函数-初始化以及启动程序入口
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int main(int argc, _TCHAR* argv[])
{
	init();			// 通过启动初始化函数开启用户接口
}

