#ifndef __MyGobangAI_H
#define __MyGobangAI_H
#include "MyGobangBase.h"
#include <cstring>
#include <thread>
#include <unordered_set>
#include <map>
#include <vector>
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
		const long long MAXSCORE = 1000000000;
		const long long BLACK_SCORE[6] = { 7, 35, 800, 15000, 800000, MAXSCORE };	//The score table of the player who is about to make a move
		const long long WHITE_SCORE[6] = { 7, 15, 400, 1800, 100000, MAXSCORE };	//The score table of the player who has just made a move

		const long double MCTS_CONFIDENCE = 0.0000001;	//the constant in the UCT formula
		const int MCTS_BOUND = 1;

		template<int N>
		class GobangAI {
		private:
			Board board[N][N] = {};
			struct ScoreManager {	//The maintainance of scores using State Compression (compresses the state of single rows: horizonal, vertical, diagonal, then adds their scores together)
				int pow3[N * 2] = { 1 };
				int* score[3][N + 1];
				int ln[N] = {}, col[N] = {}, crx[N * 2] = {}, cry[N * 2] = {};	//ln: horizonal, col: vertical, crx: diagonal (+, +), cry: diagonal (+, -)
				int crxlen[N * 2], crylen[N * 2];	//the length of diagonal rows
				int ans[3] = {};	//the score of AI / PLYR
				ScoreManager() {
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
			struct ScoreManagerManager {	//manage the scoremanager in two different situations (forgive my hilarious naming) as well as calculating the scores of compressed states
				ScoreManager ailast, plyrlast;	//ailast: AI is the player who has just made a move / plyrlast: similarly
				static void CalcScore(bool& over, ScoreManager* ailastptr, ScoreManager* plyrlastptr, int n, int l, int r) {	//Calc the score of compressed states between l and r
					ScoreManager& ailast = *ailastptr;
					ScoreManager& plyrlast = *plyrlastptr;
					for (int hash = l; hash < r; hash++) {	//I call the compressed state as "hash"
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
					over = true;
				}
				ScoreManagerManager() {	//calc the score using multi-threading
					SYSTEM_INFO sysInfo;
					GetSystemInfo(&sysInfo);
					int proccnt = sysInfo.dwNumberOfProcessors;
					for (int len = 1; len <= N; len++) {
						bool* over = new bool[proccnt]();	//the thread is ended or not
						int n = pow(3, len), m = n % proccnt, k = n / proccnt, s = 0;
						for (int i = 0; i < m; i++) {
							std::thread(CalcScore, std::ref(over[i]), &ailast, &plyrlast, len, s, s + k + 1).detach();
							s += k + 1;
						}
						int _s = s;
						s += k;
						for (int i = m + 1; i < proccnt; i++) {
							std::thread(CalcScore, std::ref(over[i]), &ailast, &plyrlast, len, s, s + k).detach();
							s += k;
						}
						CalcScore(over[m], &ailast, &plyrlast, len, _s, _s + k);	//prevent the function from being "optimized" by O2 optimization
						bool running;// one or more of the threads are still running
						do {
							running = false;
							for (int i = 0; i < proccnt; i++)
								if (!over[i]) {
									running = true;
									break;
								}
						} while (running);
						delete over;
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
			static long long GetScore(ScoreManagerManager& scoreman, Board board[N][N], Board idn, Position pos) {	//get the score when the next piece is placed
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
			struct MCTSInfo {	//the info of child node
				int all = 0;	//how many times the child node is selected/simulated
				long long score = 0x8000000000000001;	//the score of the child node
				MCTSInfo() {}
				MCTSInfo(long long _score) {
					all++;
					score = _score;
				}
			};
			struct MCTSNode {	//the node of the game tree
				int sum = 0;	//how many times the node is selected/simulated
				Position pos;
				std::map<long long, MCTSInfo> info;	//the container containing all pointers to the child nodes and their info
				MCTSNode(Position _pos) : pos(_pos) {}
				~MCTSNode() {
					for (const std::pair<long long, MCTSInfo>& pr : info)
						delete ((MCTSNode*)pr.first);
				}
				long long Build(int& now, int& depth, ScoreManagerManager& scoreman, Board board[N][N], Board idn, const std::map<long long, MCTSInfo>& lastinfo) {
					now++;
					depth = max(depth, now);
					board[pos.x][pos.y] = idn;
					if (ChkWin(board, pos.x, pos.y)) {
						board[pos.x][pos.y] = BD_NOPC;
						now--;
						return MAXSCORE;
					}
					std::unordered_set<long long> avail;
					for (const std::pair<long long, MCTSInfo>& pr : lastinfo) {
						MCTSNode& nxt = *((MCTSNode*)pr.first);
						if (nxt.pos != pos)
							avail.insert(Pos2ll(nxt.pos));
					}
					for (int i = -MCTS_BOUND; i <= MCTS_BOUND; i++)
						for (int j = -MCTS_BOUND; j <= MCTS_BOUND; j++)
							if (PointLegal(pos.x + i, pos.y + j) && board[pos.x + i][pos.y + j] == BD_NOPC)
								avail.insert(Pos2ll(Position(pos.x + i, pos.y + j)));
					if (avail.empty()) {
						board[pos.x][pos.y] = BD_NOPC;
						return 0;
					}
					scoreman.push(pos, idn);
					long long ret = 0x8000000000000001;
					for (long long ll : avail) {
						Position nxt = ll2Pos(ll);
						long long ans = GetScore(scoreman, board, Opponent(idn), nxt);
						ret = max(ret, ans);
						info[(long long)(new MCTSNode(nxt))] = MCTSInfo(ans);
					}
					sum++;
					board[pos.x][pos.y] = BD_NOPC;
					scoreman.pop(pos, idn);
					now--;
					return -ret;
				}
				long long MCTS(int& now, int& depth, ScoreManagerManager& scoreman, Board board[N][N], Board idn) {
					now++;
					board[pos.x][pos.y] = idn;
					scoreman.push(pos, idn);
					long double total = log(sum);
					long double val = -LDBL_MAX;
					MCTSNode* nxt = NULL;
					for (const std::pair<long long, MCTSInfo>& pr : info) {
						long double _val = pr.second.all ? (long double)pr.second.score / MAXSCORE + sqrt(MCTS_CONFIDENCE * total / pr.second.all) : LDBL_MAX;
						if (_val > val) {
							val = _val;
							nxt = (MCTSNode*)pr.first;
						}
					}
					long long ret = (*nxt).sum ? (*nxt).MCTS(now, depth, scoreman, board, Opponent(idn)) : (*nxt).Build(now, depth, scoreman, board, Opponent(idn), info);
					sum++;
					info[(long long)nxt].all++;
					info[(long long)nxt].score = ret;
					ret = 0x8000000000000001;
					for (const std::pair<long long, MCTSInfo>& pr : info)
						ret = max(ret, pr.second.score);
					board[pos.x][pos.y] = BD_NOPC;
					scoreman.pop(pos, idn);
					now--;
					return -ret;
				}
			}*root = NULL;
			void MoveTo(Board board[N][N], Board idn, Position pos) {	//move the root ptr to a child node of the root and delete other nodes
				MCTSNode* target = NULL;
				for (auto pr : (*root).info) {
					MCTSNode& nxt = *((MCTSNode*)pr.first);
					if (nxt.pos == pos) {
						int now, depth;
						if (nxt.sum == 0)
							nxt.Build(now, depth, scoreman, board, idn, (*root).info);
						target = &nxt;
						break;
					}
				}
				(*root).info.erase((long long)target);
				delete root;
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
				if (root == NULL) {
					root = new MCTSNode(pos);
					(*root).Build(now, depth, scoreman, board, BD_PLYR, std::map<long long, MCTSInfo>());
				}
				else {
					bool avail[N][N];
					GetAvail(board, avail, MCTS_BOUND);
					if (!avail[pos.x][pos.y])
						(*root).info[(long long)(new MCTSNode(pos))] = MCTSInfo();
					MoveTo(board, BD_PLYR, pos);
				}
				while (simtime--)
					(*root).MCTS(now, depth, scoreman, board, BD_PLYR);
				Depth = depth;
				board[pos.x][pos.y] = BD_PLYR;
				scoreman.push(pos, BD_PLYR);
				Position ret;
				long long score = 0x8000000000000001;
				for (const std::pair<long long, MCTSInfo>& pr : (*root).info) {
					if (pr.second.score > score) {
						score = pr.second.score;
						ret = (*((MCTSNode*)pr.first)).pos;
					}
				}
				Score = score;
				MoveTo(board, BD_AI, ret);
				board[ret.x][ret.y] = BD_AI;
				scoreman.push(ret, BD_AI);
				Time = GetTickCount() - start;
				return Reply(ST_NORMAL, ret.x, ret.y);
			}
			void SetPlyrFirst() {	//nothing has to be done
			}
			void SetAIFirst(int x, int y) {
				int now, depth;
				root = new MCTSNode(Position(x, y));
				(*root).Build(now, depth, scoreman, board, BD_AI, std::map<long long, MCTSInfo>());
				board[x][y] = BD_AI;
				scoreman.push(Position(x, y), BD_AI);
			}
		};
	}
}

#endif