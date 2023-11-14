#pragma once

#include "resource.h"
#include <Windows.h>
#include <tchar.h>
#include <bitset>
#include <winioctl.h>
#include <iostream>
#include <vector>
#include <string>
using namespace std;


#define BufferLength 1024

struct MBR_disk_entry
{
	uint8_t bootflag;//引导标志
	uint8_t citouhao;//磁头号
	uint8_t shanquhao;//扇区号
	uint8_t zhumianhao;//柱面号
	uint8_t disk_flag;//分区类型标志
	uint8_t someinfo[3];//id、结束磁头号、结束柱面号、结束扇区号
	uint8_t relative[4];//相对起始扇区
	uint8_t sectors[4];//总扇区数
};
struct MBR
{
	uint8_t boot_code[446];//引导代码
	//4个分区表，每个16字节,只有一个分区表有内容，对应的标志是0xEE
	MBR_disk_entry pation_table_entry[4];
	uint8_t endflag[2];//55AA
};
struct
{
	vector<string> vec;

}v;


//将四个连续字节存放的值转为int型
uint32_t transtoint(unsigned char a[])
{
	uint32_t sum = 0;
	for (int i = 0; i < 4; i++) {
		int m = a[i] / 16;
		int n = a[i] % 16;
		float len = 16;
		int temp1 = m * (pow(len, 7 - 2 * i));
		int temp2 = n * (pow(len, 6 - 2 * i));
		sum = sum + temp1 + temp2;
	}
	return sum;
}

//十进制转十六进制
string unsignedCharToHexString(unsigned char ch) {
	const char hex_chars[] = "0123456789abcdef";
	string result = "";
	unsigned int highHalfByte = (ch >> 4) & 0x0f;
	unsigned int lowHalfByte = (ch & 0x0f);
	result += hex_chars[highHalfByte];
	result += hex_chars[lowHalfByte];
	return result;
}


bool Output(MBR* mbr, char* lpBuffer, size_t len, bool ismbr, ULONGLONG* baseaddr, ULONGLONG* nextaddr, int EBRnum)
{
	bool mbrflag = 1;//在读取MBR的时候判断条目是主分区还是扩展分区条目 
	if (ismbr) {
		v.vec.push_back("第一部分(MBR):");
	}
	else {
		v.vec.push_back("////////////////////////////////////////////");
		v.vec.push_back("第" + to_string(EBRnum) + "个EBR:");
	}

	for (int i = 0; i < 446; i++) {
		mbr->boot_code[i] = lpBuffer[i];
	}
	int cnt = 446;
	for (int i = 0; i < 4; i++) {
		mbr->pation_table_entry[i].bootflag = lpBuffer[cnt];
		cnt++;
		mbr->pation_table_entry[i].citouhao = lpBuffer[cnt];
		cnt++;
		mbr->pation_table_entry[i].shanquhao = lpBuffer[cnt];
		cnt++;
		mbr->pation_table_entry[i].zhumianhao = lpBuffer[cnt];
		cnt++;
		mbr->pation_table_entry[i].disk_flag = lpBuffer[cnt];
		cnt++;
		for (int j = 0; j < 3; j++) {
			mbr->pation_table_entry[i].someinfo[j] = lpBuffer[cnt];
			cnt++;
		}
		for (int j = 0; j < 4; j++) {
			mbr->pation_table_entry[i].relative[j] = lpBuffer[cnt];
			cnt++;
		}
		for (int j = 0; j < 4; j++) {
			mbr->pation_table_entry[i].sectors[j] = lpBuffer[cnt];
			cnt++;
		}
	}
	for (int i = 0; i < 2; i++) {
		mbr->endflag[i] = lpBuffer[cnt];
		cnt++;
	}

	string mystr;
	string mystr2;
	v.vec.push_back("分区表信息解析");
	if (ismbr) {
		for (int i = 0, rank = 1; i < 4; i++, rank++) {
			if (mbr->pation_table_entry[i].disk_flag == 0x5 || mbr->pation_table_entry[i].disk_flag == 0xf) {
				v.vec.push_back("扩展分区");
				mbrflag = 0;
				rank = 4;
			}
			if (mbr->pation_table_entry[i].disk_flag < 16) {
				mystr = "第" + to_string(rank);
				mystr += "分区表标志位为: ";
				mystr += unsignedCharToHexString(mbr->pation_table_entry[i].disk_flag);
				v.vec.push_back(mystr);
			}
			else {
				mystr = "第" + to_string(rank);
				mystr += "分区表标志位为: ";
				mystr += unsignedCharToHexString(mbr->pation_table_entry[i].disk_flag);
				v.vec.push_back(mystr);
			}
			if (mbr->pation_table_entry[i].disk_flag == 0x00)//当第五位（标志位）是00时，代表分区表信息为空，无分区
			{
				//也不用往后读了 
				v.vec.push_back("分区表为空");
			}
			else {
				uint8_t center[4];
				v.vec.push_back("相对起始扇区(hex):");
				mystr = "";
				for (int j = 0, k = 3; j < 4; j++, k--) {
					mystr += unsignedCharToHexString(mbr->pation_table_entry[i].relative[k]);
					mystr += " ";
					center[j] = mbr->pation_table_entry[i].relative[k];
				}
				v.vec.push_back(mystr);
				uint32_t tempadd = transtoint(center);
				v.vec.push_back("开始地址为(扇区)：");
				v.vec.push_back(to_string(tempadd));

				if (ismbr && !mbrflag)// if in mbr and got a extend entry,the EBR at relsecor+nowbase(0)
				{
					*baseaddr = (ULONGLONG)tempadd + (*baseaddr);//only change once
					*nextaddr = (ULONGLONG)0UL;
					//*nextaddr = (ULONGLONG)tempadd;
				}

				v.vec.push_back("大小(hex)：");
				mystr = "";
				for (int j = 0, k = 3; j < 4; j++, k--) {
					mystr += unsignedCharToHexString(mbr->pation_table_entry[i].sectors[k]);
					mystr += " ";
					center[j] = mbr->pation_table_entry[i].sectors[k];
				}
				v.vec.push_back(mystr);
				//计算大小，转化为GB单位
				uint32_t tempsize = transtoint(center);
				if (ismbr && !mbrflag) {
					v.vec.push_back("扩展盘总大小为：" + to_string(tempsize) + "扇区 = " + to_string(((double)tempsize / 2.0 / 1024.0 / 1024.0)) + "GB");
				}
				else if (!ismbr) {
					v.vec.push_back("第" + to_string(EBRnum) + "逻辑盘大小为：" + to_string(tempsize) + "扇区 = " + to_string(((double)tempsize / 2.0 / 1024.0 / 1024.0)) + "GB");
				}
				else {
					v.vec.push_back("该盘大小为：" + to_string(tempsize) + "扇区 = " + to_string(((double)tempsize / 2.0 / 1024.0 / 1024.0)) + "GB");
				}
			}
		}
	}
	else {
		bool notfin = true;
		int cnt = 0;
		for (; cnt < 2;) {
			mystr = "";
			if (mbr->pation_table_entry[cnt].disk_flag == 0x5) {
				mbrflag = 0;
				notfin = false;
			}
			if (mbr->pation_table_entry[cnt].disk_flag < 16) {
				mystr += "分区表标志位为: ";
				mystr += unsignedCharToHexString(mbr->pation_table_entry[cnt].disk_flag);
				v.vec.push_back(mystr);
			}
			else {
				mystr += "分区表标志位为: ";
				mystr += unsignedCharToHexString(mbr->pation_table_entry[cnt].disk_flag);
				v.vec.push_back(mystr);
			}
			if (mbr->pation_table_entry[cnt].disk_flag == 0x0) {
				v.vec.push_back("////////no next extend patition!/////////");
				mbrflag = 1;
				notfin = false;
			}
			else {
				uint8_t center[4];
				if (cnt == 0) {
					v.vec.push_back("相对起始扇区(hex):");
					mystr = "";
					for (int j = 0, k = 3; j < 4; j++, k--) {
						mystr += unsignedCharToHexString(mbr->pation_table_entry[cnt].relative[k]);
						mystr += " ";
						center[j] = mbr->pation_table_entry[cnt].relative[k];
					}
					v.vec.push_back(mystr);

					uint32_t tempadd = transtoint(center);
					v.vec.push_back("开始地址为(扇区)：");
					v.vec.push_back(to_string((ULONGLONG)tempadd + (*nextaddr) + (*baseaddr)));

					v.vec.push_back("大小(hex)：");
					mystr = "";
					for (int j = 0, k = 3; j < 4; j++, k--) {
						mystr += unsignedCharToHexString(mbr->pation_table_entry[cnt].sectors[k]);
						mystr += " ";
						center[j] = mbr->pation_table_entry[cnt].sectors[k];
					}
					v.vec.push_back(mystr);
				}
				else {
					v.vec.push_back("下一分区起始扇区(hex):");
					mystr = "";
					for (int j = 0, k = 3; j < 4; j++, k--) {
						mystr += unsignedCharToHexString(mbr->pation_table_entry[cnt].relative[k]);
						mystr += " ";
						center[j] = mbr->pation_table_entry[cnt].relative[k];
					}
					v.vec.push_back(mystr);

					uint32_t tempadd = transtoint(center);
					v.vec.push_back("开始地址为(扇区)：");
					v.vec.push_back(to_string((ULONGLONG)tempadd + (*baseaddr)));
					*nextaddr = (ULONGLONG)tempadd;

					v.vec.push_back("大小(hex)：");
					mystr = "";
					for (int j = 0, k = 3; j < 4; j++, k--) {
						mystr += unsignedCharToHexString(mbr->pation_table_entry[cnt].sectors[k]);
						mystr += " ";
						center[j] = mbr->pation_table_entry[cnt].sectors[k];
					}
					v.vec.push_back(mystr);

					//计算大小，转化为GB单位
					uint32_t tempsize = transtoint(center);
					if (ismbr && !mbrflag) {
						v.vec.push_back("扩展盘总大小为：" + to_string(tempsize) + "扇区 = " + to_string(((double)tempsize / 2.0 / 1024.0 / 1024.0)) + "GB");
					}
					else if (!ismbr) {
						v.vec.push_back("第" + to_string(EBRnum) + "逻辑盘大小为：" + to_string(tempsize) + "扇区 = " + to_string(((double)tempsize / 2.0 / 1024.0 / 1024.0)) + "GB");
					}
					else {
						v.vec.push_back("该盘大小为：" + to_string(tempsize) + "扇区 = " + to_string(((double)tempsize / 2.0 / 1024.0 / 1024.0)) + "GB");
					}
				}
			}
			cnt++;
		}
	}
	return (mbrflag);
}

bool GetDriveMsg(DISK_GEOMETRY* pdg, int addr)
{
	HANDLE hDevice;               // 设备句柄
	BOOL bResult;                 // results flag
	DWORD junk;                   // discard resultscc
	char lpBuffer[BufferLength] = { 0 };
	MBR* mbr = new MBR;


	//通过CreateFile来获得设备的句柄
	hDevice = CreateFile(TEXT("\\\\.\\PhysicalDrive1"), // 设备名称
		GENERIC_READ,                // no access to the drive
		FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
		NULL,             // default security attributes
		OPEN_EXISTING,    // disposition
		0,                // file attributes
		NULL);            // do not copy file attributes
	if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
	{
		return (FALSE);
	}

	//通过DeviceIoControl函数与设备进行IO
	bResult = DeviceIoControl(hDevice, // 设备的句柄
		IOCTL_DISK_GET_DRIVE_GEOMETRY, // 控制码，指明设备的类型
		NULL,
		0, // no input buffer
		pdg,
		sizeof(*pdg),
		&junk,                 // # bytes returned
		(LPOVERLAPPED)NULL); // synchronous I/O

	LARGE_INTEGER offset;//long long signed
	offset.QuadPart = (ULONGLONG)addr * (ULONGLONG)512;//a sector
	SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);//从0开始读MBR
	if (GetLastError())
		return (FALSE);//如果出错了

	DWORD dwCB;
	//从这个位置开始读 
	BOOL bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);

	bool finished = 0;
	int EBRnum = 0;
	ULONGLONG* baseaddr = new ULONGLONG, * nextaddr = new ULONGLONG;//扩展分区起始地址，EBR地址 
	*baseaddr = (ULONGLONG)0;
	*nextaddr = (ULONGLONG)0;
	finished = Output(mbr, lpBuffer, 512, true, baseaddr, nextaddr, EBRnum);

	if (finished)
		CloseHandle(hDevice);
	else
	{
		//继续读
		do {
			EBRnum++;
			memset(lpBuffer, 0, sizeof(lpBuffer));
			offset.QuadPart = (ULONGLONG)((*baseaddr) * ((ULONGLONG)512) + (*nextaddr) * ((ULONGLONG)512));//find the EBR
			SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);//读EBR
			ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
		} while (!Output(mbr, lpBuffer, 512, false, baseaddr, nextaddr, EBRnum));
		CloseHandle(hDevice);
	}

	delete mbr;
	delete baseaddr;
	delete nextaddr;
	return bResult;
}
