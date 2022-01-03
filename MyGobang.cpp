#include "MyGobang.h"
#include <cstdlib>
#include <cstring>
using namespace Gobang;
using namespace Gobang::AI;
using namespace Gobang::GUI;

const bool DEBUG = true;	//whether the result(check the PrintResult function) should be printed

const int GOBANG_SIZE = 15;	//the size of the chessboard
const int EAZY = 25000, NORMAL = 50000, HARD = 100000;	//how many times the simulation will run before a move
const int DIFFICULTY = HARD;

const wchar_t INITING_TITLE[] = L"初始化中，请稍后...";
const wchar_t WINDOW_TITLE[] = L"这真的是一个非常非常好玩有趣的五子棋";
const wchar_t WIN_CONTENT[] = L"恭喜你，你存留了人类的希望火种！";
const wchar_t WIN_TITLE[] = L"你赢了";
const wchar_t LOSE_CONTENT[] = L"永不服输的人类，执起手中的棋子再战一盘吧！";
const wchar_t LOSE_TITLE[] = L"你被AI打败了";
const wchar_t CHOICE_CONTENT[] = L"选 是 则玩家先手，选 否 则AI先手。";	//which one moves first, PLYR or AI
const wchar_t CHOICE_TITLE[] = L"是否玩家先手？";

void PrintResult(GobangGUI<GOBANG_SIZE, GOBANG_SIZE>& gui, GobangAI<GOBANG_SIZE>& ai) {
	if (!DEBUG)
		return;
	char time[12], depth[12], score[12];
	_itoa_s(ai.Time, time, 10);
	_itoa_s(ai.Depth, depth, 10);
	_itoa_s(ai.Score, score, 10);
	wchar_t msg[36] = {};
	int len = 0;
	for (int i = 0; i < strlen(time); i++)
		msg[len++] = time[i];
	msg[len++] = L' ';
	for (int i = 0; i < strlen(depth); i++)
		msg[len++] = depth[i];
	msg[len++] = L' ';
	for (int i = 0; i < strlen(score); i++)
		msg[len++] = score[i];
	gui.SendOKBox(msg, L"用时 & 深度 & 得分");
}

void PlyrFirst(GobangGUI<GOBANG_SIZE, GOBANG_SIZE>& gui, GobangAI<GOBANG_SIZE>& ai) {
	ai.SetPlyrFirst();
	gui.SetWindowTitle(WINDOW_TITLE);
	while (true) {
		auto guistat = gui.GetLatestAction();
		if (guistat.state == GUI::ST_CLICK && ai.ChkPos(guistat.pos) == BD_NOPC) {
			gui.Place(IDN_BLACK, guistat.pos);
			auto aistat = ai.GetMove(guistat.pos, DIFFICULTY);
			PrintResult(gui, ai);
			switch (aistat.state) {
			case AI::ST_NORMAL:
				gui.Place(IDN_WHITE, aistat.pos);
				break;
			case AI::ST_PLYRWIN:
				gui.SendOKBox(WIN_CONTENT, WIN_TITLE);
				return;
			case AI::ST_AIWIN:
				gui.Place(IDN_WHITE, aistat.pos);
				gui.SendOKBox(LOSE_CONTENT, LOSE_TITLE);
				return;
			default:
				break;
			}
		}
	}
}

void AIFirst(GobangGUI<GOBANG_SIZE, GOBANG_SIZE>& gui, GobangAI<GOBANG_SIZE>& ai) {
	ai.SetAIFirst(GOBANG_SIZE >> 1, GOBANG_SIZE >> 1);
	gui.SetWindowTitle(WINDOW_TITLE);
	gui.Place(IDN_BLACK, Position(GOBANG_SIZE >> 1, GOBANG_SIZE >> 1));
	while (true) {
		auto guistat = gui.GetLatestAction();
		if (guistat.state == GUI::ST_CLICK && ai.ChkPos(guistat.pos) == BD_NOPC) {
			gui.Place(IDN_WHITE, guistat.pos);
			auto aistat = ai.GetMove(guistat.pos, DIFFICULTY);
			PrintResult(gui, ai);
			switch (aistat.state) {
			case AI::ST_NORMAL:
				gui.Place(IDN_BLACK, aistat.pos);
				break;
			case AI::ST_PLYRWIN:
				gui.SendOKBox(WIN_CONTENT, WIN_TITLE);
				return;
			case AI::ST_AIWIN:
				gui.Place(IDN_BLACK, aistat.pos);
				gui.SendOKBox(LOSE_CONTENT, LOSE_TITLE);
				return;
			default:
				break;
			}
		}
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	GobangGUI<GOBANG_SIZE, GOBANG_SIZE> gui(INITING_TITLE);	//GUI MUST BE DECLARED INSIDE A FUNCTION (or nothing will be displayed)
	GobangAI<GOBANG_SIZE> ai;	//It is recommended to declare the gui befor the ai (otherwise the app won't display anything until the initialization is done)
	switch (gui.SendYNBox(CHOICE_CONTENT, CHOICE_TITLE)) {
	case 1:
		PlyrFirst(gui, ai);
		break;
	case 0:
		AIFirst(gui, ai);
		break;
	}
	return 0;
}