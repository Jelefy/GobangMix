#ifndef __MyGobangAI_H
#define __MyGobangAI_H
#include "MyGobangBase.h"
#include <cstring>
#include <vector>
#include <stack>
#include <set>
#include <cmath>
#include <cfloat>
#include <windows.h>

namespace Gobang {
	namespace AI {
		typedef unsigned char Board;
		typedef unsigned char State;

		const Board BD_NOPC = 0;
		const Board BD_PLYR = 1;
		const Board BD_AI = 2;

		Board Opponent(Board identity) {
			switch (identity) {
			case BD_PLYR:
				return BD_AI;
			case BD_AI:
				return BD_PLYR;
			default:
				return BD_NOPC;
			}
		}

		const State ST_CANTPLACE = -1;
		const State ST_NORMAL = 0;
		const State ST_PLYRWIN = 1;
		const State ST_AIWIN = 2;

		const int DIRCNT = 4;
		const int DIR[DIRCNT][2] = { {1, 0}, {0, 1}, {1, 1}, {1, -1} };
		const int MAXSCORE = 10000000;
		const int BLACK_SCORE[6] = { 7, 35, 800, 15000, 800000, MAXSCORE };
		const int WHITE_SCORE[6] = { 7, 15, 400, 1800, 100000, MAXSCORE };

		const long double UCT_CONFIDENCE = 0.002;	//the constants in the UCT/LCT formula
		const long double LCT_CONFIDENCE = 0.002;

		const int MCTS_BOUND = 1;

		template<int N>
		class GobangAI {
		private:
			Board board[N][N] = {};
			struct ScoreMaintenance {	//The maintainance of scores using State Compression (compresses the state of single rows: horizonal, vertical, diagonal, then adds their scores together)
				int pow3[N * 2] = { 1 };
				int* score[3][N + 1];
				int ln[N] = {}, col[N] = {}, crx[N * 2] = {}, cry[N * 2] = {};	//ln: horizonal, col: vertical, crx: diagonal (+, +), cry: diagonal (+, -)
				int crxlen[N * 2], crylen[N * 2];	//the length of diagonal rows
				int ans[3] = {};	//the score of AI / PLYR
				ScoreMaintenance() {
					for (int i = 1; i < N * 2; i++)
						pow3[i] = pow3[i - 1] * 3;
					for (int i = 1; i < 3; i++)
						for (int j = 1; j <= N; j++)
							score[i][j] = new int[pow(3, j)]();
					for (int i = 1; i <= N; i++) {
						crxlen[i] = i;
						crxlen[2 * N - i] = i;
						crylen[i - 1] = i;
						crylen[2 * N - i - 1] = i;
					}
				}
				void push(Position pos, Board idn) {	//a piece is placed
					int idln = pos.x, atln = pos.y;	//id: the number of the row, at: the number of the place the piece is "at" in that row
					int idcol = pos.y, atcol = pos.x;
					int idcrx = pos.x - pos.y + N, atcrx = min(pos.x, pos.y);
					int idcry = pos.x + pos.y, atcry = (idcry >= N) ? pos.x - (idcry - N + 1) : pos.x;
					for (int i = 1; i < 3; i++)
						ans[i] -= score[i][N][ln[idln]] + score[i][N][col[idcol]] + score[i][crxlen[idcrx]][crx[idcrx]] + score[i][crylen[idcry]][cry[idcry]];	//When one piece is changed, 4 rows will be changed, including 1 horizonal, 1 vertical, 2 diagonal
					ln[idln] += pow3[atln] * idn;
					col[idcol] += pow3[atcol] * idn;
					crx[idcrx] += pow3[atcrx] * idn;
					cry[idcry] += pow3[atcry] * idn;
					for (int i = 1; i < 3; i++)
						ans[i] += score[i][N][ln[idln]] + score[i][N][col[idcol]] + score[i][crxlen[idcrx]][crx[idcrx]] + score[i][crylen[idcry]][cry[idcry]];
				}
				void pop(Position pos, Board idn) {	//unplace a placed piece
					int idln = pos.x, atln = pos.y;
					int idcol = pos.y, atcol = pos.x;
					int idcrx = pos.x - pos.y + N, atcrx = min(pos.x, pos.y);
					int idcry = pos.x + pos.y, atcry = (idcry >= N) ? pos.x - (idcry - N + 1) : pos.x;
					for (int i = 1; i < 3; i++)
						ans[i] -= score[i][N][ln[idln]] + score[i][N][col[idcol]] + score[i][crxlen[idcrx]][crx[idcrx]] + score[i][crylen[idcry]][cry[idcry]];	//When one piece is changed, 4 rows will be changed, including 1 horizonal, 1 vertical, 2 diagonal
					ln[idln] -= pow3[atln] * idn;
					col[idcol] -= pow3[atcol] * idn;
					crx[idcrx] -= pow3[atcrx] * idn;
					cry[idcry] -= pow3[atcry] * idn;
					for (int i = 1; i < 3; i++)
						ans[i] += score[i][N][ln[idln]] + score[i][N][col[idcol]] + score[i][crxlen[idcrx]][crx[idcrx]] + score[i][crylen[idcry]][cry[idcry]];
				}
			};
			struct ScoreManager {	//manage the scoremanager in two different situations (forgive my hilarious naming) as well as calculating the scores of compressed states
				ScoreMaintenance ailast, plyrlast;	//ailast: AI is the player who has just made a move / plyrlast: similarly
				ScoreManager() {	//calc the score
					for (int n = 1; n <= N; n++)
						for (int hash = pow(3, n) - 1; hash >= 0; hash--) {	//I call the compressed state as "hash"
							int tmp = hash;
							Board* pc = new Board[n];	//pieces of the row
							for (int i = 0; i < n; i++) {
								pc[i] = tmp % 3;
								tmp /= 3;
							}
							for (int i = 0; i + 5 <= n; i++) {
								Board idn = BD_NOPC;
								int cnt = 0;
								for (int j = i; j < i + 5; j++)
									if (pc[j] != BD_NOPC) {
										if (idn == Opponent(pc[j])) {
											cnt = -1;
											break;
										}
										idn = pc[j];
										cnt++;
									}
								if (idn == BD_NOPC) {
									plyrlast.score[BD_AI][n][hash] += BLACK_SCORE[0];
									plyrlast.score[BD_PLYR][n][hash] += WHITE_SCORE[0];
									ailast.score[BD_AI][n][hash] += WHITE_SCORE[0];
									ailast.score[BD_PLYR][n][hash] += BLACK_SCORE[0];
								}
								else if (cnt != -1) {
									plyrlast.score[idn][n][hash] += idn == BD_AI ? BLACK_SCORE[cnt] : WHITE_SCORE[cnt];
									ailast.score[idn][n][hash] += idn == BD_AI ? WHITE_SCORE[cnt] : BLACK_SCORE[cnt];
								}
							}
							delete pc;
						}
				}
				void push(Position pos, Board idn) {
					ailast.push(pos, idn);
					plyrlast.push(pos, idn);
				}
				void pop(Position pos, Board idn) {
					ailast.pop(pos, idn);
					plyrlast.pop(pos, idn);
				}
				int getscore(Board idn) {	//return the score of the correct situation
					return idn == BD_AI ? ailast.ans[BD_AI] - ailast.ans[BD_PLYR] : plyrlast.ans[BD_PLYR] - plyrlast.ans[BD_AI];
				}
			}scoreman;
			static bool PointLegal(int x, int y) {	//the cordinate is inside the chessboard or not
				return x >= 0 && x < N&& y >= 0 && y < N;
			}
			static void GetAvail(Board board[N][N], bool avail[N][N], int bound) {	//get the scope of the combination of the 3x3 scopes of each placed piece
				memset(avail, 0, sizeof(bool) * N * N);
				for (int x = 0; x < N; x++)
					for (int y = 0; y < N; y++) if (board[x][y] != BD_NOPC)
						for (int i = -bound; i <= bound; i++)
							for (int j = -bound; j <= bound; j++)
								if (PointLegal(x + i, y + j))
									avail[x + i][y + j] = true;
			}
			static bool ChkWin(Board board[N][N], int x, int y) {	//check if the last placed piece formed a 5-in-a-row
				for (int i = 0; i < DIRCNT; i++) {
					Position l(x, y), r(x, y);
					int cnt = 1;
					while (cnt < 5 && PointLegal(l.x - DIR[i][0], l.y - DIR[i][1]) && board[l.x - DIR[i][0]][l.y - DIR[i][1]] == board[x][y]) {
						cnt++;
						l.x -= DIR[i][0], l.y -= DIR[i][1];
					}
					while (cnt < 5 && PointLegal(r.x + DIR[i][0], r.y + DIR[i][1]) && board[r.x + DIR[i][0]][r.y + DIR[i][1]] == board[x][y]) {
						cnt++;
						r.x += DIR[i][0], r.y += DIR[i][1];
					}
					if (cnt == 5)
						return true;
				}
				return false;
			}
			static Position ScanVulAI(Board board[N][N], Board identity) {	//check if there exists a win in 1 step
				for (int x = 0; x < N; x++)
					for (int y = 0; y < N; y++) if (board[x][y] != Opponent(identity))
						for (int k = 0, s, nx, ny; k < DIRCNT; k++) {
							int cnt = 0;
							for (s = 0, nx = x, ny = y; s < 5 && PointLegal(nx, ny) && board[nx][ny] != Opponent(identity); s++, nx += DIR[k][0], ny += DIR[k][1])
								if (board[nx][ny] == identity) cnt++;
							if (s == 5 && cnt == 4) {
								nx = x, ny = y;
								while (board[nx][ny] != BD_NOPC)
									nx += DIR[k][0], ny += DIR[k][1];
								return Position(nx, ny);
							}
						}
				return Position(-1, -1);
			}
			int GetScore(Board idn, Position pos) {	//get the score when the next piece is placed
				board[pos.x][pos.y] = idn;
				if (ChkWin(board, pos.x, pos.y)) {
					board[pos.x][pos.y] = BD_NOPC;
					return MAXSCORE;
				}
				scoreman.push(pos, idn);
				int ret = scoreman.getscore(idn);
				board[pos.x][pos.y] = BD_NOPC;
				scoreman.pop(pos, idn);
				return ret;
			}
			struct MCTSNode {
				Position pos;
				int sim = 1, score;
				int head = 0, nxt;
				MCTSNode() {}
				MCTSNode(Position _pos, int _score, int _nxt) : pos(_pos), score(_score), nxt(_nxt) {}
			};
			std::vector<MCTSNode> node = std::vector<MCTSNode>(1);
			std::stack<int> nodestk;
			int NewNode(Position pos, int score, int nxt) {
				if (nodestk.empty()) {
					node.push_back(MCTSNode(pos, score, nxt));
					return node.size() - 1;
				}
				int ret = nodestk.top();
				nodestk.pop();
				node[ret] = MCTSNode(pos, score, nxt);
				return ret;
			}
			void CreateChild(int id, Position pos, int score) {
				int ret = NewNode(pos, score, node[id].head);
				node[id].head = ret;
			}
			void DeleteTree(int id) {
				nodestk.push(id);
				for (int i = node[id].head; i; i = node[i].nxt)
					DeleteTree(i);
			}
			int Build(int id, Board idn, int fa) {
				Position pos = node[id].pos;
				board[pos.x][pos.y] = idn;
				if (ChkWin(board, pos.x, pos.y)) {
					board[pos.x][pos.y] = BD_NOPC;
					return MAXSCORE;
				}
				std::set<Position> avail;
				for (int i = node[fa].head; i; i = node[i].nxt)
					if (node[i].pos != pos)
						avail.insert(node[i].pos);
				for (int i = -MCTS_BOUND; i <= MCTS_BOUND; i++)
					for (int j = -MCTS_BOUND; j <= MCTS_BOUND; j++)
						if (PointLegal(pos.x + i, pos.y + j) && board[pos.x + i][pos.y + j] == BD_NOPC)
							avail.insert(Position(pos.x + i, pos.y + j));
				if (avail.empty()) {
					board[pos.x][pos.y] = BD_NOPC;
					return 0;
				}
				scoreman.push(pos, idn);
				int ret = 0x80000001;
				for (Position nxt : avail) {
					int ans = GetScore(Opponent(idn), nxt);
					ret = max(ret, ans);
					CreateChild(id, nxt, ans);
				}
				node[id].sim++;
				board[pos.x][pos.y] = BD_NOPC;
				scoreman.pop(pos, idn);
				return -ret;
			}
			int MCTS(int& now, int& depth, int id, Board idn) {
				now++;
				depth = max(now + 1, depth);
				Position pos = node[id].pos;
				board[pos.x][pos.y] = idn;
				scoreman.push(pos, idn);
				long double total = log(node[id].sim);
				long double val = -LDBL_MAX;
				int nxt;
				for (int i = node[id].head; i; i = node[i].nxt) {
					long double _val = (long double)node[i].score / MAXSCORE + sqrt(UCT_CONFIDENCE * total / node[i].sim);
					if (_val > val) {
						val = _val;
						nxt = i;
					}
				}
				int ret = node[nxt].head ? MCTS(now, depth, nxt, Opponent(idn)) : Build(nxt, Opponent(idn), id);
				node[id].sim++;
				node[nxt].score = ret;
				ret = 0x80000001;
				for (int i = node[id].head; i; i = node[i].nxt)
					ret = max(ret, node[i].score);
				board[pos.x][pos.y] = BD_NOPC;
				scoreman.pop(pos, idn);
				now--;
				return -ret;
			}
			int root = 0;
			void MoveTo(Board idn, Position pos) {	//move the root ptr to a child node of the root and delete other nodes
				int target;
				if (node[node[root].head].pos == pos) {
					target = node[root].head;
					node[root].head = node[node[root].head].nxt;
				}
				else for(int i = node[root].head; i; i = node[i].nxt)
					if (node[node[i].nxt].pos == pos) {
						target = node[i].nxt;
						node[i].nxt = node[node[i].nxt].nxt;
					}
				if (!node[target].head)
					Build(target, idn, root);
				DeleteTree(root);
				root = target;
			}

		public:
			int Time, Depth, Score;
			struct Reply {
				State state;
				Position pos;
				Reply(State _state, int x, int y) : state(_state), pos(Position(x, y)) {}
				Reply(State _state) : state(_state) {}
			};
			Board ChkPos(Position pos) {
				return board[pos.x][pos.y];
			}
			Reply GetMove(Position pos, int simtime) {
				Time = Depth = Score = 0;
				int start = GetTickCount();
				int now = 0, depth = 0;
				if (board[pos.x][pos.y] != BD_NOPC)
					return Reply(ST_CANTPLACE);
				board[pos.x][pos.y] = BD_PLYR;
				if (ChkWin(board, pos.x, pos.y))
					return Reply(ST_PLYRWIN);
				Position vul = ScanVulAI(board, BD_AI);
				if (vul.x != -1)
					return Reply(ST_AIWIN, vul.x, vul.y);
				board[pos.x][pos.y] = BD_NOPC;
				if (!root) {
					root = NewNode(pos, 0, 0);
					Build(root, BD_PLYR, 0);
				}
				else {
					bool avail[N][N];
					GetAvail(board, avail, MCTS_BOUND);
					if (!avail[pos.x][pos.y])
						CreateChild(root, pos, 0);
					MoveTo(BD_PLYR, pos);
				}
				while (simtime--)
					MCTS(now, depth, root, BD_PLYR);
				Depth = depth;
				board[pos.x][pos.y] = BD_PLYR;
				scoreman.push(pos, BD_PLYR);
				Position ret;
				long double total = log((long double)node[root].sim);
				long double val = -LDBL_MAX;
				int score = 0x80000001;
				for (int i = node[root].head; i; i = node[i].nxt) {
					long double _val = (long double)node[i].score / MAXSCORE - sqrt(LCT_CONFIDENCE * total / node[i].sim);
					if (_val > val) {
						val = _val;
						score = node[i].score;
						ret = node[i].pos;
					}
				}
				Score = score;
				MoveTo(BD_AI, ret);
				board[ret.x][ret.y] = BD_AI;
				scoreman.push(ret, BD_AI);
				Time = GetTickCount() - start;
				return Reply(ST_NORMAL, ret.x, ret.y);
			}
			void SetPlyrFirst() {	//nothing has to be done
			}
			void SetAIFirst(int x, int y) {
				root = NewNode(Position(x, y), 0, 0);
				Build(root, BD_AI, 0);
				board[x][y] = BD_AI;
				scoreman.push(Position(x, y), BD_AI);
			}
		};
	}
}

#endif