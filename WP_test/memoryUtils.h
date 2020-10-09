/*
 * ���ڴ沿��Դ������https://pvz.lmintlcx.com/memory/, https://github.com/lmintlcx/pvztools/
 * ԭ���ߣ�LmintLCX
*/

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <iostream>
#include <initializer_list>
#include <Windows.h>
#include "pvzstruct.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")

#ifndef MEM
#define MEM
struct Memory {

	HWND hwnd = nullptr;     // ���ھ��
	DWORD pid = 0;           // ���̱�ʶ
	HANDLE handle = nullptr; // ���̾��
	int level = -1;				// ����ɹ���
	bool isQX = false;	// �Ƿ�����б������

	// ��ȡ�ڴ�ĺ���ģ��
	template <typename T>
	T ReadMemory(std::initializer_list<uintptr_t> addr) {
		T result = T();
		uintptr_t buffer = 0;
		for (auto it = addr.begin(); it != addr.end(); it++) {
			if (it != addr.end() - 1) {
				unsigned long read_size = 0;
				int ret = ReadProcessMemory(handle,                       //
					(const void*)(buffer + *it), //
					&buffer,                      //
					sizeof(buffer),               //
					&read_size);                  //
				if (ret == 0 || sizeof(buffer) != read_size)
					return T();
			}
			else {
				unsigned long read_size = 0;
				int ret = ReadProcessMemory(handle,                       //
					(const void*)(buffer + *it), //
					&result,                      //
					sizeof(result),               //
					&read_size);                  //
				if (ret == 0 || sizeof(result) != read_size)
					return T();
			}
		}
		return result;
	}

	// д���ڴ��ģ��
	template <typename T>
	void WriteMemory(T value, std::initializer_list<uintptr_t> addr) {
		uintptr_t offset = 0;
		for (auto it = addr.begin(); it != addr.end(); it++) {
			if (it != addr.end() - 1) {
				unsigned long read_size = 0;
				int ret = ReadProcessMemory(handle, (const void*)(offset + *it), &offset, sizeof(offset), &read_size);
				if (ret == 0 || sizeof(offset) != read_size)
					return;
			}
			else {
				unsigned long write_size = 0;
				int ret = WriteProcessMemory(handle, (void*)(offset + *it), &value, sizeof(value), &write_size);
				if (ret == 0 || sizeof(value) != write_size)
					return;
			}
		}
	}

	// ��鵱ǰ�����Ƿ�Ϊ��б - ��Ҫ8���ң�9�ش̣� ʣ�¾�Ϊ��/С��������Ϊ8
	bool checkQX(int** plants) {
		if (countPlant(plants, YT_29) == 8 && (countPlant(plants, DC_21) == 9 &&
			(countPlant(plants, XRK_1) + countPlant(plants, XPG_8) == 8))) {
			return true;
		}
		return false;
	}

	int countPlant(int** plants, int idx) {
		int sum = 0;
		for (int i = 0; i < 5; i++)
			for (int j = 0; j < 5; j++)
				if (plants[i][j] == idx) {
					sum++;
				}
		return sum;
	}

	// ������Ϸ���ڴ򿪽��̾��
	bool FindGame() {
		hwnd = FindWindowW(L"MainWindow", L"Plants vs. Zombies");
		if (hwnd == nullptr)
			return false;

		GetWindowThreadProcessId(hwnd, &pid);
		if (pid == 0)
			return false;

		handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
		return handle != nullptr;
	}

	bool FindGame2() {
		hwnd = FindWindowW(L"MainWindow", L"ֲ���ս��ʬ���İ�");
		if (hwnd == nullptr)
			return false;

		GetWindowThreadProcessId(hwnd, &pid);
		if (pid == 0)
			return false;

		handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
		return handle != nullptr;
	}

	bool GameOn() {
		return ReadMemory<int>({ 0x6a9ec0 }) != 0;
	}

	// �ر���Ϸ���̾��
	void CloseGame() {
		if (handle != nullptr)
			CloseHandle(handle);
	}

	int** readPlants(bool setRunInBackground, bool autoCollect) {
		if (!FindGame())
			if (!FindGame2())
				return nullptr;

		if (GameOn()) {
			// ��̨����
			if (setRunInBackground) {
				WriteMemory<short>(0x00eb, { 0x0054eba8 });
			}
			else {
				WriteMemory<short>(0x2e74, { 0x0054eba8 });
			}

			// �Զ��ռ�
			if (autoCollect) {
				WriteMemory<byte>(0xeb, { 0x0043158f });
			}
			else {
				WriteMemory<byte>(0x75, { 0x0043158f });
			}
		}

		// ��ȡ��Ϸģʽ
		auto gamemode = ReadMemory<int>({ 0x6a9ec0, 0x7f8 });
		if (gamemode != 70) {
			return nullptr;
		}

		// ��ȡ��Ϸ���棨3 - ��IZE�ڲ���
		auto gameui = ReadMemory<int>({ 0x6a9ec0, 0x7FC });
		if (gameui != 3) {
			return nullptr;
		}

		// ��ȡ����
		bool levelChange = false;
		int currentLevel = ReadMemory<int>({ 0x6a9ec0, 0x768, 0x160, 0x6c });
		if (currentLevel != level) {
			level = currentLevel;
			levelChange = true;
		}

		int** puzzle = new int* [5];
		for (int i = 0; i < 5; i++)
			puzzle[i] = new int[5];
		for (int i = 0; i < 5; i++)
			for (int j = 0; j < 5; j++)
				puzzle[i][j] = -1;

		// ��ȡ����ֲ����Ϣ
		auto plants_offset = ReadMemory<unsigned int>({ 0x6a9ec0, 0x768, 0xac });
		auto plants_count_max = ReadMemory<unsigned int>({ 0x6a9ec0, 0x768, 0xb0 });
		for (size_t i = 0; i < plants_count_max; ++i) {
			auto plant_dead = ReadMemory<bool>({ plants_offset + 0x141 + 0x14c * i });
			auto plant_squished = ReadMemory<bool>({ plants_offset + 0x142 + 0x14c * i });
			auto plant_type = ReadMemory<int>({ plants_offset + 0x24 + 0x14c * i });
			auto plant_row = ReadMemory<int>({ plants_offset + 0x1c + 0x14c * i });
			auto plant_col = ReadMemory<int>({ plants_offset + 0x28 + 0x14c * i });
			if (!plant_dead && !plant_squished) {
				if (plant_row >= 0 && (plant_row <= 4 && (plant_col >= 0 && plant_col <= 4))) {
					puzzle[plant_row][plant_col] = plant_type;
				}
			}
		}
		if (levelChange || !isQX) {
			isQX = checkQX(puzzle);
			levelChange = false;
		}
		CloseGame();
		return puzzle;
	}
};
#endif