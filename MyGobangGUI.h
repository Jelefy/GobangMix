#ifndef __MyGobangGUI_H
#define __MyGobangGUI_H

#include "MyGobangBase.h"
#include <graphics.h>

namespace Gobang {
	namespace GUI {
		typedef bool Identity;
		typedef bool State;

		const Identity IDN_BLACK = 0;
		const Identity IDN_WHITE = 1;

		const State ST_CLICK = 1;
		const State ST_NONE = 0;

		const int BLKSZ = 33;
		const int HALFBLK = BLKSZ >> 1;
		const int PCRAD = HALFBLK * 2 / 3;
		const int LNRAD = 0;

		const COLORREF BKCOLOR = RGB(248, 203, 100);
		const COLORREF LNCOLOR = RGB(255, 255, 255);

		template<int N, int M>
		class GobangGUI {
		private:
			HWND hwnd;
			int Pos2Raw(int pos) {
				return HALFBLK + pos * BLKSZ;
			}
			int Raw2Pos(int raw) {
				return raw / BLKSZ;
			}
			Identity idn;
			Position pos = Position(-1, -1);

		public:
			GobangGUI(const wchar_t* title) {
				hwnd = initgraph(BLKSZ * M, BLKSZ * N);
				SetWindowText(hwnd, title);
				setbkcolor(BKCOLOR);
				cleardevice();
				setfillcolor(LNCOLOR);
				for (int i = 0; i < N; i++) {
					int x = Pos2Raw(i);
					for (int y = Pos2Raw(0); y <= Pos2Raw(M - 1); y++)
						solidcircle(x, y, LNRAD);
				}
				for (int j = 0; j < M; j++) {
					int y = Pos2Raw(j);
					for (int x = Pos2Raw(0); x <= Pos2Raw(N - 1); x++)
						solidcircle(x, y, LNRAD);
				}
			}
			~GobangGUI() {
				closegraph();
			}
			void SetWindowTitle(const wchar_t* title) {
				SetWindowText(hwnd, title);
			}
			void Place(Identity identity, Position position) {
				if (pos.x != -1) {
					switch (idn) {
					case IDN_BLACK:
						setlinecolor(DARKGRAY);
						break;
					case IDN_WHITE:
						setlinecolor(LIGHTGRAY);
						break;
					}
					circle(pos.x, pos.y, PCRAD);
				}
				idn = identity;
				pos = Position(Pos2Raw(position.x), Pos2Raw(position.y));
				switch (identity) {
				case IDN_BLACK:
					setfillcolor(BLACK);
					break;
				case IDN_WHITE:
					setfillcolor(WHITE);
					break;
				}
				setlinecolor(RED);
				solidcircle(pos.x, pos.y, PCRAD);
				circle(pos.x, pos.y, PCRAD);
			}
			struct Reply {
				State state;
				Position pos;
				Reply(State _st, int x, int y) : state(_st), pos(Position(x, y)) {}
				Reply(State _st) : state(_st) {}
			};
			Reply GetLatestAction() {
				ExMessage msg;
				if (!peekmessage(&msg, EM_MOUSE) || msg.message != WM_LBUTTONUP)
					return Reply(ST_NONE);
				return Reply(ST_CLICK, Raw2Pos(msg.x), Raw2Pos(msg.y));
			}
			void SendOKBox(const wchar_t* content, const wchar_t* title) {
				MessageBox(hwnd, content, title, MB_OK);
			}
			int SendYNBox(const wchar_t* content, const wchar_t* title) {
				switch (MessageBox(hwnd, content, title, MB_YESNO)) {
				case IDYES:
					return 1;
				case IDNO:
					return 0;
				default:
					return -1;
				}
			}
		};
	}
}

#endif