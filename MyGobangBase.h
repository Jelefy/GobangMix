#ifndef __MYGOBANGBASE_H
#define __MYGOBANGBASE_H

namespace Gobang {
	struct Position {
		int x, y;
		Position(int _x, int _y) : x(_x), y(_y) {}
		Position() {}
		bool operator==(const Position &b) const{
			return x == b.x && y == b.y;
		}
		bool operator!=(const Position &b) const{
			return x != b.x || y != b.y;
		}
	};
	long long Pos2ll(Position pos) {
		return *((long long*)&pos);
	}
	Position ll2Pos(long long ll) {
		return *((Position*)&ll);
	}
}

#endif